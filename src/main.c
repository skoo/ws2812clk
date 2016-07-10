#include "stm32f0xx_conf.h"
#include "i2c_xfer.h"
#include "pcf8523.h"
#include "tsl2572.h"
#include "ws2812_spi.h"
#include "led_power.h"
#include "systick.h"
#include "prog_if.h"
#include "usart.h"
#include "clockface.h"
#include <string.h>

#include "stm32f0xx_spi.h"
#include "stm32f0xx_dma.h"

volatile uint32_t last_brg_value = 0;

int _err = 0;

void gpio_init(void)
{
	GPIO_InitTypeDef gi;

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOF, ENABLE);

	gi.GPIO_Pin = GPIO_Pin_1;
	gi.GPIO_Mode = GPIO_Mode_IN;
	gi.GPIO_PuPd = GPIO_PuPd_NOPULL;
	gi.GPIO_Speed = GPIO_Speed_2MHz;

	GPIO_Init(GPIOA, &gi);	// PA1: Light sensor interrupt
	GPIO_Init(GPIOB, &gi);	// PB1: RTC interrupt
}

int main(void)
{
    systick_init();

    usart_init(115200);
	gpio_init();
	i2c_init();
	led_power_init(0);

    delay_ms(500);
    pcf8523_init();

	led_power_enable(1);
    ws2812_init(0);

    int no_light_sensor = 1;

    do
    {
	    if (tsl2572_init() == 0)
	    {
	    	no_light_sensor = 0;
	        prog_if_init();
	    }
	    else
	    {
	        /* light sensor was not found */
	        int i;
	        for (i = 0; i < 5; i++)
	        {
	            clockface_fill(16, 0, 0);
	            ws2812_update();
	            delay_ms(333);
	            clockface_fill(0, 0, 0);
	            ws2812_update();
	        }
	    }
	} while (no_light_sensor);

    rtc_time_t clock;

    uint8_t prev_sec = 0xff;

	while (1)
	{
		last_brg_value = tsl2572_read_brightness();

		int ret = pcf8523_get_time(&clock);
		if (ret == 0)
		{
		    if (prev_sec != clock.sec)
		    {
		        prev_sec = clock.sec;
		        clockface_draw(&clock);
		    }
		}
		else if (ret == 1)
		{
		    // problem with oscillator
		    clockface_fill(0, 0, 8);
		}
		else
		{
		    // i2c problem
		    clockface_fill(8, 0, 0);
		}

		if (prog_if_set_time_trigger > 0)
		{
	        /* set new time */
		    prog_if_set_time_trigger = 0;
		    pcf8523_write_regs(RTC_TIME_REG_OFFSET_SEC, prog_if_get_prog_data(), 3);
            clockface_fill(0, 8, 8);
            ws2812_update();
            delay_ms(500);
		}

		ws2812_update();
	}
}
