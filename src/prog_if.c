#include "prog_if.h"
#include "tsl2572.h"
#include "prog_if.h"
#include "usart.h"
#include "pcf8523.h"

#include "stm32f0xx_conf.h"
#include "stm32f0xx_usart.h"

/* reset pulse 50ms on + 50ms off */
#define RESET_PULSE 50
#define RESET_PULSE_THRESHOLD 40

/* bit on  : 150ms + 50ms
 * bit off : 50ms + 150ms
 */
#define DATA_PULSE 200
#define DATA_PULSE_THRESHOLD 50
#define DATA_PULSE_ON 150
#define DATA_PULSE_OFF 50

/**
 *
 * Pulses:
 *
 * 5 x (50ms on / 50ms off) reset pulses
 *
 * 4 x 8 data bits LSB first:
 *   '0': 50ms on / 150ms off
 *	 '1': 150ms on / 50ms off
 *
 * Data:
 *
 *	0xFF bcd_sec bcd_min bcd_hour
 *
 */

#define DEBUG

#ifdef DEBUG
#define DBG(x) usart_print(x)
#define DBG_CHR(x) usart_put(x)
#define DBG_HEX(x) usart_print_hex(x)
#else
#define DBG(x)
#define DBG_CHR(x)
#define DBG_HEX(x)
#endif

#define ENABLE_BRG_HISTORY
#define ENABLE_PULSE_HISTORY

#define MS2TICKS(x) ((x)/10)
#define TICKS2MS(x) ((x)*10)

#define WAVG(v1,w1,v2,w2) (((v1)*(w1)+(v2)*(w2))/(w1+w2))

extern volatile uint32_t last_brg_value;
volatile uint32_t prog_if_set_time_trigger = 0;

static uint8_t _rx_data[8];
static int _bytes_received = 0;

static uint32_t low_point = 0;
static uint32_t high_point = 1000;

#ifdef ENABLE_BRG_HISTORY
#define BRG_HIST_SIZE 1024
static volatile uint16_t brg_values[BRG_HIST_SIZE];
static volatile uint16_t brg_idx = 0;
#endif

#ifdef ENABLE_PULSE_HISTORY
#define PULSE_HIST_SIZE 100
static volatile uint16_t pulse_values[PULSE_HIST_SIZE];
static volatile uint16_t pulse_idx = 0;
#endif

static void prog_if(uint32_t brightness);

void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET) {
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);

	    /* Ignore program pulses while previously received
	     * clock set command is not handled in main loop.
	     */
	    if (prog_if_set_time_trigger == 0)
	    {
	    	uint32_t brightness = last_brg_value;
	#ifdef ENABLE_BRG_HISTORY
			brg_values[brg_idx] = brightness;

			if (++brg_idx == BRG_HIST_SIZE)
			{
				brg_idx = 0;
			}
	#endif
	        prog_if(brightness);
	    }
	}
}

void prog_if_init(void)
{
    NVIC_InitTypeDef ni;
    TIM_TimeBaseInitTypeDef ti;
    
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    // 5000 counts in 20ms -> 4us
    ti.TIM_CounterMode = TIM_CounterMode_Up;
    ti.TIM_ClockDivision = TIM_CKD_DIV1; // 24Mhz
    ti.TIM_Prescaler = 6*1000; // 4kHz
    ti.TIM_Period = 80; // 100Hz

    TIM_TimeBaseInit(TIM2, &ti);

    ni.NVIC_IRQChannel = TIM2_IRQn;
    ni.NVIC_IRQChannelPriority = 1;
    ni.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&ni);

    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
    TIM_Cmd(TIM2, ENABLE);
}

const uint8_t* prog_if_get_prog_data(void)
{
	return _rx_data+1;
}

static void handle_pulse(uint32_t bit_on_time, uint32_t bit_off_time)
{
    static int bit_pos = 0;
	static int started = 0;
    static int start_reset_counter = 0;
    static uint8_t data_byte = 0;

	uint32_t bit_time = bit_on_time + bit_off_time;

#ifdef ENABLE_PULSE_HISTORY
	pulse_values[pulse_idx++] = bit_on_time;
	pulse_values[pulse_idx++] = bit_off_time;

	if (pulse_idx == PULSE_HIST_SIZE)
	{
		pulse_idx = 0;
	}
#endif	

	/* start */
	if (bit_time >= RESET_PULSE*2 - RESET_PULSE_THRESHOLD &&
		bit_time <= RESET_PULSE*2 + RESET_PULSE_THRESHOLD)
	{
	    if (++start_reset_counter == 5)
	    {
	    	start_reset_counter = 0;
	    	started = 1,
	    	bit_pos = 0;
	    	data_byte = 0;

	    	_bytes_received = 0;
	    }
	}
	else if (started)
	{
	    /* data bits */
	    if (bit_time >= DATA_PULSE - DATA_PULSE_THRESHOLD &&
	    	bit_time <= DATA_PULSE + DATA_PULSE_THRESHOLD)
	    {
	        if (bit_on_time > bit_time/2)
	        {
	            data_byte |= (1 << bit_pos);
	        }

	        if (++bit_pos == 8)
	        {
	            _rx_data[_bytes_received++] = data_byte;
	            if ((_rx_data[0] = 0xff) && (_bytes_received == 4))
	            {
	                prog_if_set_time_trigger++;
	                started = 0;
	            }
	            else if (_bytes_received == 8)
	            {
	                _bytes_received = 0;
	                started = 0;
	            }
	                                        
	            bit_pos = 0;
	            data_byte = 0;
	        }
	    }
	    else
	    {
	        /* incorrect bit code length */
	        started = 0;
	    }
	}
	else
	{
		start_reset_counter = 0;
	}
}

void prog_if(uint32_t brightness)
{
	static uint32_t prev_brightness = 0;
	static uint32_t state_on = 0;
	static uint32_t len = 0;
	static uint32_t len_on = 0;
	static int dir = 0;

	/* adjust brightness min/max values (threshold) */

	if (brightness > high_point)
	{
		high_point = WAVG(high_point, 7, brightness, 1);
	}

	if (brightness < low_point)
	{
		low_point = WAVG(low_point, 7, brightness, 1);
	}

	if (brightness > prev_brightness)
	{
		if (dir < 0)
		{
			low_point = WAVG(low_point, 15, prev_brightness, 1);
		}
		dir = 1;
	}
	else if (brightness < prev_brightness)
	{
		if (dir > 0)
		{
				high_point = WAVG(high_point, 15, prev_brightness, 1);
		}
		dir = -1;
	}

	/* check brightness against current threshold */

	if (brightness >= WAVG(low_point, 2, high_point, 1))
	{
		if (!state_on)
		{
			// off -> on
			handle_pulse(TICKS2MS(len_on), TICKS2MS(len));
			state_on = 1;
			len = 0;
		}

		len++;
	}
	else
	{
		if (state_on)
		{
			// on -> off
			state_on = 0;
			len_on = len;
			len = 0;
		}

		len++;
	}

	prev_brightness = brightness;
}
