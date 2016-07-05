#include "stm32f0xx_conf.h"
#include "stm32f0xx_i2c.h"
#include "systick.h"
#include "i2c_xfer.h"

#define XFER_MODE_NACK    (1 << 6)
#define XFER_MODE_DONE    (1 << 7)

static i2c_xfer_t* _xfer = 0;
static i2c_xfer_t* _xfer_data = 0;
static volatile int _xfer_in_progress = 0;

void I2C1_IRQHandler(void)
{
	if (I2C_GetITStatus(I2C1, I2C_IT_TXIS) == SET) {
		if (_xfer && (_xfer->mode & XFER_MODE_DONE) == 0) {
            _xfer_data->pos++;

		    if (_xfer_data->pos == _xfer_data->len && _xfer_data->next)
		        _xfer_data = _xfer_data->next;

			if (_xfer_data->pos < _xfer_data->len) {
				I2C_SendData(I2C1, _xfer_data->buffer[_xfer_data->pos]);
			}
		}
		I2C_ClearITPendingBit(I2C1, I2C_IT_TXIS);
	}

    if (I2C_GetITStatus(I2C1, I2C_IT_RXNE) == SET) {
        if (_xfer && (_xfer->mode & XFER_MODE_DONE) == 0) {
            if (_xfer_data->pos == _xfer_data->len && _xfer_data->next)
                _xfer_data = _xfer_data->next;

            if (_xfer_data->pos < _xfer_data->len) {
                _xfer_data->buffer[_xfer_data->pos++] = I2C_ReceiveData(I2C1);
            }
        }
        I2C_ClearITPendingBit(I2C1, I2C_IT_RXNE);
    }

	if (I2C_GetITStatus(I2C1, I2C_IT_TC) == SET) {
		I2C_ClearITPendingBit(I2C1, I2C_IT_TC);
		I2C_GenerateSTOP(I2C1, ENABLE);
	}

    if (I2C_GetITStatus(I2C1, I2C_IT_STOPF) == SET) {
        I2C_ClearITPendingBit(I2C1, I2C_IT_STOPF);
        if (_xfer)
        {
            _xfer->mode |= XFER_MODE_DONE;
            _xfer = 0;
        }
        _xfer_in_progress = 0;
    }

	if (I2C_GetITStatus(I2C1, I2C_IT_NACKF) == SET) {
		I2C_ClearITPendingBit(I2C1, I2C_IT_NACKF);
		if (_xfer)
		{
            _xfer->mode |= XFER_MODE_NACK;
            _xfer->mode |= XFER_MODE_DONE;
            _xfer = 0;
		}
        _xfer_in_progress = 0;
	}
}

void i2c_init(void)
{
	GPIO_InitTypeDef gi;
	I2C_InitTypeDef ii;
	NVIC_InitTypeDef ni;

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_4);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_4);

    RCC_I2CCLKConfig(RCC_I2C1CLK_HSI);

    gi.GPIO_OType = GPIO_OType_PP;
    gi.GPIO_PuPd = GPIO_PuPd_NOPULL;
    gi.GPIO_Mode =GPIO_Mode_OUT;
    gi.GPIO_Pin = GPIO_Pin_9;
    gi.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gi);

    /* Clock out possible I2C transaction left active
     * when mcu was restarted (no way to reset I2C slaves). */

    int i;
    for (i = 0; i < 10; i++)
    {
        GPIO_ResetBits(GPIOA, GPIO_Pin_9);
        delay_ms(1);
        GPIO_SetBits(GPIOA, GPIO_Pin_9);
        delay_ms(1);
    }

    gi.GPIO_OType = GPIO_OType_OD;
    gi.GPIO_PuPd = GPIO_PuPd_NOPULL;
    gi.GPIO_Mode = GPIO_Mode_AF;
    gi.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
    gi.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gi);

    // 400kHz@8Mhz PRESC=0, SCLL=0x09, SCLH=0x03, SDADEL=0x1, SCLDEL=0x03
    //ii.I2C_Timing = (0x00 << 28) | (0x03 << 20) | (0x01 << 16) | (0x03 << 8) | 0x09;
    // 100kHz@8MHz PRESC=1, 0x13, 0x0f, 0x02, 0x04
    ii.I2C_Timing = (0x02 << 28) | (0x13 << 20) | (0x0f << 16) | (0x02 << 8) | 0x04;

    ii.I2C_Mode = I2C_Mode_I2C;
    ii.I2C_OwnAddress1 = 0;
    ii.I2C_Ack = I2C_Ack_Disable;
    ii.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    ii.I2C_AnalogFilter = I2C_AnalogFilter_Enable;
    ii.I2C_DigitalFilter = 0;

	I2C_Init(I2C1, &ii);

	ni.NVIC_IRQChannel = I2C1_IRQn;
	ni.NVIC_IRQChannelPriority = 1;
	ni.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&ni);

	I2C_ITConfig(I2C1, I2C_IT_TXI | I2C_IT_RXI | I2C_IT_NACKI | I2C_IT_TCI | I2C_IT_STOPI, ENABLE);

    I2C_Cmd(I2C1, ENABLE);
}

int i2c_xfer_is_done(void)
{
	return ((_xfer->mode & XFER_MODE_DONE) != 0);
}

void i2c_xfer_wait_done(void)
{
    while (_xfer_in_progress);
}

void i2c_xfer_setup(i2c_xfer_t* xfer, uint8_t mode, uint8_t* buffer, uint8_t len, i2c_xfer_t* next)
{
    xfer->mode = mode;
    xfer->buffer = buffer;
    xfer->len = len;
    xfer->pos = 0;
    xfer->next = next;
}

void i2c_xfer_start(uint8_t addr, i2c_xfer_t* xfer)
{
    i2c_xfer_wait_done();
    _xfer_in_progress = 1;
    _xfer = xfer;
    _xfer_data = xfer;

    i2c_xfer_t* x = xfer;
    int len = 0;
    do
    {
        len += x->len;
        x = x->next;
    } while (x);

    I2C_NumberOfBytesConfig(I2C1, len);

    if (xfer->mode & XFER_MODE_RX)
    {
        I2C_MasterRequestConfig(I2C1, I2C_Direction_Receiver);
        I2C_SlaveAddressConfig(I2C1, addr | 0x01);
    }
    else
    {
        I2C_MasterRequestConfig(I2C1, I2C_Direction_Transmitter);
        I2C_SlaveAddressConfig(I2C1, addr);
        I2C_SendData(I2C1, _xfer->buffer[_xfer->pos]);
    }

    I2C_GenerateSTART(I2C1, ENABLE);
}

int i2c_xfer_run(uint8_t addr, i2c_xfer_t* xfer)
{
    i2c_xfer_start(addr, xfer);
    i2c_xfer_wait_done();
    return ((xfer->mode & XFER_MODE_NACK) == 0 ? 0 : -1);
}
