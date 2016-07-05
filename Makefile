PROJ_NAME=clk

STM32F0_DEVICE = STM32F050

# LD_DEVICE, needed for RAM & FLASH definitions.
LD_DEVICE = stm32f050x4_def.ld

# Clock source to use, either:
# PLL_SOURCE_HSI (Internal) or PLL_SOURCE_HSE (External)
PLLSOURCE = PLL_SOURCE_HSI

SYSCLK_HZ = 48000000

# Sources
SRCS = clockface.c i2c_xfer.c led_power.c main.c pcf8523.c prog_if.c systick.c tsl2572.c usart.c ws2812_spi.c

#CFLAG_OPT = -Os

# makefile that does the actual work
include $(STM32BASE)/stm32base.mk

