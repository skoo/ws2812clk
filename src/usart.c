#include "stm32f0xx_conf.h"
#include "stm32f0xx_usart.h"
#include "stm32f0xx_rcc.h"
#include "usart.h"

void usart_init(int baud)
{
    USART_InitTypeDef ui;
    GPIO_InitTypeDef gi;
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
    RCC_USARTCLKConfig(RCC_USART1CLK_PCLK);
    
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_1);
    
    gi.GPIO_OType = GPIO_OType_PP;
    gi.GPIO_Mode = GPIO_Mode_AF;
    gi.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
    gi.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOA, &gi);
    
    USART_StructInit(&ui);
    ui.USART_BaudRate = baud;
    USART_Init(USART1, &ui);
    USART_Cmd(USART1, ENABLE);
}

void usart_put(char chr)
{
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    USART_SendData(USART1, chr);
}

void usart_print(const char* str)
{
    while (*str)
    {
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
        USART_SendData(USART1, *str++);
    }
}

void usart_print_hex_char(uint8_t v)
{
    const char hex[16] = {
            '0', '1', '2', '3', '4', '5', '6', '7',
            '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
    };

    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    USART_SendData(USART1, hex[v >> 4]);
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    USART_SendData(USART1, hex[v & 0x0f]);
}

void usart_print_hex(uint16_t v)
{
    usart_print_hex_char(v >> 8);
    usart_print_hex_char(v & 0x00ff);
}