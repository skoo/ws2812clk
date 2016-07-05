#include "stm32f0xx_conf.h"
#include "stm32f0xx_i2c.h"
#include "tsl2572.h"
#include "i2c_xfer.h"
#include "systick.h"

#define GAIN_1      0x00
#define GAIN_8      0x01
#define GAIN_16     0x02
#define GAIN_120    0x03

#define TSL2572_ADDRESS (0x39 << 1)

static int tsl2572_read_regs(uint8_t reg,  uint8_t* buffer, uint8_t num)
{
    i2c_xfer_t cmd;
    uint8_t cmd_byte = (reg & 0x1f) | 0x80 | 0x20;
    i2c_xfer_setup(&cmd, 0, &cmd_byte, 1, 0);
    if (i2c_xfer_run(TSL2572_ADDRESS, &cmd) != 0)
        return -1;

    i2c_xfer_setup(&cmd, XFER_MODE_RX, buffer, num, 0);
    return (i2c_xfer_run(TSL2572_ADDRESS, &cmd) == 0 && cmd.pos == num ? 0 : -2);
}

static int tsl2572_write_reg(uint8_t reg, uint8_t value)
{
    i2c_xfer_t cmd;
    uint8_t cmd_data[2] = { (reg & 0x1f) | 0x80 | 0x20, value };

    i2c_xfer_setup(&cmd, 0, cmd_data, 2, 0);
    return i2c_xfer_run(TSL2572_ADDRESS, &cmd);
}

static int tsl2572_read_channel(uint8_t channel)
{
    uint8_t data[2];
    if (tsl2572_read_regs(0x14 + (channel&1)*2, data, 2) == 0)
    {
        return ((int)(data[1] << 8) | (int)data[0]);
    }
    else
    {
        return -1;
    }
}

int tsl2572_read_lux(void)
{
    const int ATIME = 11; // 11ms (10.92ms) as set in init.
    const int GAIN = 1; // 120x as set in init.
    const int GA = 1; // glass attenuation, 1 for no attenuation.
    const int CPL = (ATIME*GAIN) / (GA * 60);

    int ch0 = tsl2572_read_channel(0);
    int ch1 = tsl2572_read_channel(1);

    if (ch0 < 0 || ch1 < 0)
        return -1;

    ch0 *= 100;
    ch1 *= 100;

    int lux1 = (ch0 - 187 * ch1) / (CPL*100);
    int lux2 = (63 * ch0 - ch1) / (CPL*100);

    if (lux2 > lux1)
        lux1 = lux2;

    return (lux1 > 0 ? lux1 / 100 : 0);
}

int tsl2572_read_brightness(void)
{
    return (tsl2572_read_channel(0));
}

int tsl2572_init(void)
{
    uint8_t id;

    if (tsl2572_read_regs(0x12, &id, 1) == 0 && (id == 0x34 || id == 0x3d))
    {
        tsl2572_write_reg(0x00, 0x00); // power off, als disable
        delay_ms(10);
        tsl2572_write_reg(0x00, 0x01); // power on
        delay_ms(100);
        tsl2572_write_reg(0x01, 249); // ATIME=~20ms for rejecting 50Hz ripple
        tsl2572_write_reg(0x0f, GAIN_120);
        return tsl2572_write_reg(0x00, 0x03); // power on, als enable
    }
    else
    {
        return -1;
    }

}
