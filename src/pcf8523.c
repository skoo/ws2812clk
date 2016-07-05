#include "stm32f0xx_conf.h"
#include "i2c_xfer.h"
#include "pcf8523.h"
#include "systick.h"

#define PCF8523_ADDRESS 0xd0

void pcf8523_init(void)
{
	const uint8_t control[] =
	{
			0x80, // 12.5pF, clock running, 24h
			0x00, // no watch timers, interrupts
			0x00, // battery switch over and battery low detection
	};

	pcf8523_write_regs(RTC_CONTROL_REGS_OFFSET, control, sizeof(control));
}

static void bcd_to_dec(uint8_t* b)
{
    uint8_t v = (*b >> 4) * 10 + (*b & 0x0f);
    *b = v;
}

int pcf8523_get_time(rtc_time_t* t)
{
	int ret = pcf8523_read_regs(RTC_TIME_REG_OFFSET, (void*)t, sizeof(rtc_time_t));
	if (ret == 0)
	{
	    if (t->sec & 0x80)
	    {
	        // oscillator has not settled, try to clear the flag
            uint8_t s = t->sec & 0x7f;
            pcf8523_write_regs(RTC_TIME_REG_OFFSET_SEC, &s, 1);
	        ret = 1;
	    }
	    t->sec &= 0x7f;

	    bcd_to_dec(&t->sec);
	    bcd_to_dec(&t->min);
	    bcd_to_dec(&t->hour);
	    bcd_to_dec(&t->month);
	    bcd_to_dec(&t->year);
	}
	return ret;
}

int pcf8523_write_regs(uint8_t reg, const uint8_t* buffer, uint8_t num)
{
    i2c_xfer_t cmd;
    i2c_xfer_t data;

    i2c_xfer_setup(&cmd, 0, &reg, 1, &data);
    i2c_xfer_setup(&data, 0, (uint8_t*)buffer, num, 0);
    return i2c_xfer_run(PCF8523_ADDRESS, &cmd);
}

int pcf8523_read_regs(uint8_t reg, uint8_t* buffer, uint8_t num)
{
    i2c_xfer_t cmd;

    i2c_xfer_setup(&cmd, 0, &reg, 1, 0);
    if (i2c_xfer_run(PCF8523_ADDRESS, &cmd) != 0)
        return -1;

    i2c_xfer_setup(&cmd, XFER_MODE_RX, buffer, num, 0);
    return (i2c_xfer_run(PCF8523_ADDRESS, &cmd) == 0 && cmd.pos == num ? 0 : -2);
}
