#include "stm32f0xx_conf.h"
#include "stm32f0xx_spi.h"
#include "stm32f0xx_dma.h"
#include "ws2812_spi.h"
#include <string.h>

/* Use 3MHz SPI (4-bits for single ws2812 bit), long pulse timing 999ns
 * is very close to specified maximum of 1000ns.
 *
 * Uses 6MHz SPI (8-bits for single ws2812 bit) if not defined,
 * timings are better within specified limits.
 */
#define USE_3MHz_CLK

#ifdef USE_3MHz_CLK

/* SPI @ 3Mhz (48MHz/16):
 * short pulse 333ns (1 bits)
 * long pulse  999ns (3 bits)
 * total      1333ns (4 bits)
 *
 * "0": 1000 0x8
 * "1": 1110 0xe
 *
 * Actual output is inverted (FET for 3.3V -> 5V level transition, pin not 5v-tolerant),
 * so:
 *
 * "0": 0111 0x7
 * "1": 0001 0x1
 */

#define RESET_PULSE_BYTE_COUNT 20
#define CODE_BIT_COUNT 4

/* Inverted bits to send with SPI for WS2812 "0" and "1" codes */
#define WS2812_BIT_ON_H   0x10    /* 0001 ---_ */
#define WS2812_BIT_OFF_H  0x70    /* 0111 -___ */

#define WS2812_BIT_ON_L   0x01    /* 0001 ---_ */
#define WS2812_BIT_OFF_L  0x07    /* 0111 -___ */

#define WS2812_BIT_OFF    0x77

#else

/* SPI @ 6Mhz (48MHz/8):
 * short pulse 500ns (3 bits)
 * long pulse  833ns (5 bits)
 * total      1333ns (8 bits)
 *
 * "0": 11100000 0xe0
 * "1": 11111000 0xf8
 *
 * Actual output is inverted (FET for 3.3V -> 5V level transition, pin not 5v-tolerant),
 * so:
 *
 * "0": 00011111 0x1f
 * "1": 00000111 0x07
  */

#define RESET_PULSE_BYTE_COUNT 40
#define CODE_BIT_COUNT 8

/* Inverted bits to send with SPI for WS2812 "0" and "1" codes */
#define WS2812_BIT_ON   0x07    /* 00000111 -----___ */
#define WS2812_BIT_OFF  0x1f    /* 00011111 ---_____ */

#endif

/*
 * Buffer for 60 RGB LED values + "off" bytes (~53us) for reset.
 */
static uint8_t ws2812_buffer[60*3*CODE_BIT_COUNT+RESET_PULSE_BYTE_COUNT];

static int _auto_update = 0;
static volatile int _update_ongoing;

static void start_transfer(void* ptr, uint32_t len)
{
	_update_ongoing = 1;

	DMA_Cmd(DMA1_Channel3, DISABLE);

	DMA1_Channel3->CMAR = (uint32_t)ptr;
	DMA1_Channel3->CNDTR = len;

	DMA_Cmd(DMA1_Channel3, ENABLE);
	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE);
}

void DMA1_Channel2_3_IRQHandler(void)
{
	if (DMA_GetITStatus(DMA1_IT_TC3) == SET) {
		DMA_ClearITPendingBit(DMA1_IT_TC3);
		_update_ongoing = 0;
		if (_auto_update)
		{
			/* Trigger new DMA transfer */
			start_transfer(&ws2812_buffer, sizeof(ws2812_buffer));
		}
	}
}

static void ws2812_spi_init(void)
{
	SPI_InitTypeDef si;
	GPIO_InitTypeDef gi;
	DMA_InitTypeDef dmi;
	NVIC_InitTypeDef ni;

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

	GPIO_PinAFConfig(GPIOA, GPIO_PinSource7, GPIO_AF_0);

	/* MOSI */
	gi.GPIO_Pin = GPIO_Pin_7;
	gi.GPIO_Mode = GPIO_Mode_AF;
	gi.GPIO_OType = GPIO_OType_PP;
	gi.GPIO_PuPd = GPIO_PuPd_NOPULL;
	gi.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_Init(GPIOA, &gi);

	/* WS2812:
	 * short pulse 400ns +- 150  = 250..550ns
	 * long pulse  850ns +- 150  = 700..1000ns
	 * "0" / "1"  1250ns +- 300  = 950..1550ns
	 * reset >50us
	 */

	SPI_StructInit(&si);
	si.SPI_Mode = SPI_Mode_Master;
	si.SPI_NSS = SPI_NSS_Soft;
#ifdef USE_3MHz_CLK
	si.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16; // 3Mbit/s
#else
	si.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8; // 6Mbit/s
#endif
	si.SPI_Direction = SPI_Direction_1Line_Tx;
	SPI_I2S_DeInit(SPI1);
	SPI_Init(SPI1, &si);

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

	dmi.DMA_DIR = DMA_DIR_PeripheralDST;
	dmi.DMA_M2M = DMA_M2M_Disable;
	dmi.DMA_Mode = DMA_Mode_Normal;
	dmi.DMA_Priority = DMA_Priority_High;
	dmi.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	dmi.DMA_MemoryInc = DMA_MemoryInc_Enable;
	dmi.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	dmi.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	dmi.DMA_PeripheralBaseAddr = (uint32_t)&(SPI1->DR);
	dmi.DMA_MemoryBaseAddr = (uint32_t)&ws2812_buffer;
	dmi.DMA_BufferSize = sizeof(ws2812_buffer);
	DMA_Init(DMA1_Channel3, &dmi);

	ni.NVIC_IRQChannel = DMA1_Channel2_3_IRQn;
	ni.NVIC_IRQChannelPriority = 1;
	ni.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&ni);
	DMA_ITConfig(DMA1_Channel3, DMA_IT_TC, ENABLE);

	DMA_Cmd(DMA1_Channel3, ENABLE);

	/* Trigger the first DMA transfer, following ones are triggered in DMA IRQ handler */
	SPI_I2S_ClearFlag(SPI1, SPI_I2S_FLAG_TXE);
	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE);
	SPI_Cmd(SPI1, ENABLE);

	if(_auto_update)
		start_transfer(&ws2812_buffer, sizeof(ws2812_buffer));
}

#ifdef USE_3MHz_CLK

#define WSB00 ((uint16_t)(WS2812_BIT_OFF_H | WS2812_BIT_OFF_L))
#define WSB01 ((uint16_t)(WS2812_BIT_OFF_H | WS2812_BIT_ON_L))
#define WSB10 ((uint16_t)(WS2812_BIT_ON_H  | WS2812_BIT_OFF_L))
#define WSB11 ((uint16_t)(WS2812_BIT_ON_H  | WS2812_BIT_ON_L))

#define WSH(x) (x << 8)
#define WSL(x) (x)

void ws2812_led(uint32_t pos, uint8_t r, uint8_t g, uint8_t b)
{
    static const uint16_t bits[] =
    {
            WSH(WSB00) | WSL(WSB00),
            WSH(WSB01) | WSL(WSB00),
            WSH(WSB10) | WSL(WSB00),
            WSH(WSB11) | WSL(WSB00),

            WSH(WSB00) | WSL(WSB01),
            WSH(WSB01) | WSL(WSB01),
            WSH(WSB10) | WSL(WSB01),
            WSH(WSB11) | WSL(WSB01),

            WSH(WSB00) | WSL(WSB10),
            WSH(WSB01) | WSL(WSB10),
            WSH(WSB10) | WSL(WSB10),
            WSH(WSB11) | WSL(WSB10),

            WSH(WSB00) | WSL(WSB11),
            WSH(WSB01) | WSL(WSB11),
            WSH(WSB10) | WSL(WSB11),
            WSH(WSB11) | WSL(WSB11),
    };

    uint16_t* ptr = (uint16_t*)(ws2812_buffer + pos*3*4 + 10);

    *ptr-- = bits[b & 0x0f];
    *ptr-- = bits[b >> 4];

    *ptr-- = bits[r & 0x0f];
    *ptr-- = bits[r >> 4];

    *ptr-- = bits[g & 0x0f];
    *ptr   = bits[g >> 4];
}

#else

static void ws2812_set(uint8_t* ptr, uint8_t v)
{
	*ptr++ = v & 0x80 ? WS2812_BIT_ON : WS2812_BIT_OFF; v <<= 1;
	*ptr++ = v & 0x80 ? WS2812_BIT_ON : WS2812_BIT_OFF; v <<= 1;
	*ptr++ = v & 0x80 ? WS2812_BIT_ON : WS2812_BIT_OFF; v <<= 1;
	*ptr++ = v & 0x80 ? WS2812_BIT_ON : WS2812_BIT_OFF; v <<= 1;

	*ptr++ = v & 0x80 ? WS2812_BIT_ON : WS2812_BIT_OFF; v <<= 1;
	*ptr++ = v & 0x80 ? WS2812_BIT_ON : WS2812_BIT_OFF; v <<= 1;
	*ptr++ = v & 0x80 ? WS2812_BIT_ON : WS2812_BIT_OFF; v <<= 1;
	*ptr++ = v & 0x80 ? WS2812_BIT_ON : WS2812_BIT_OFF; v <<= 1;
}

void ws2812_led(uint32_t pos, uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t* ptr = ws2812_buffer + pos*3*CODE_BIT_COUNT;

    ws2812_set(ptr, g);
    ws2812_set(ptr+CODE_BIT_COUNT*2, b);
    ws2812_set(ptr+CODE_BIT_COUNT, r);
}

#endif

void ws2812_init(int auto_update)
{
	_auto_update = auto_update;

	memset(ws2812_buffer, WS2812_BIT_OFF, sizeof(ws2812_buffer)-RESET_PULSE_BYTE_COUNT);
	memset(ws2812_buffer+sizeof(ws2812_buffer)-RESET_PULSE_BYTE_COUNT, 0xff, RESET_PULSE_BYTE_COUNT);
	ws2812_spi_init();
}

void ws2812_update(void)
{
	if (!_auto_update)
	{
		while (_update_ongoing);
		
		/* Trigger new DMA transfer */
		start_transfer(&ws2812_buffer, sizeof(ws2812_buffer));
	}
}