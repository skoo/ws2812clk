#ifndef I2C_XFER_H_
#define I2C_XFER_H_

typedef struct _I2C_XFER
{
    uint8_t* buffer;
    uint8_t len;
    uint8_t mode;
    uint8_t pos;

    void *next;
} i2c_xfer_t;

#define XFER_MODE_RX    (1 << 0)

void i2c_init(void);

void i2c_xfer_setup(i2c_xfer_t* xfer, uint8_t mode, uint8_t* buffer, uint8_t len, i2c_xfer_t* next);
void i2c_xfer_start(uint8_t addr, i2c_xfer_t* xfer);
int i2c_xfer_run(uint8_t addr, i2c_xfer_t* xfer);

int i2c_xfer_is_done(void);
void i2c_xfer_wait_done(void);

#endif // I2C_XFER_H_
