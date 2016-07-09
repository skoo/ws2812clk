#ifndef WS2812_SPI_H_
#define WS2812_SPI_H_

void ws2812_init(int autoupdate);
void ws2812_update(void);
void ws2812_led(uint32_t pos, uint8_t r, uint8_t g, uint8_t b);

#endif // WS2812_SPI_H_
