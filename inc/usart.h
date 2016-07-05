#ifndef USART_H_
#define USART_H_

void usart_init(int baud);
void usart_put(char chr);
void usart_print(const char* str);
void usart_print_hex_char(uint8_t v);
void usart_print_hex(uint16_t v);

#endif // USART_H_
