#ifndef WS2812_SPI_H_
#define WS2812_SPI_H_

void ws2812_init(void);
void ws2812_led(uint32_t pos, uint8_t r, uint8_t g, uint8_t b);

#endif // WS2812_SPI_H_
