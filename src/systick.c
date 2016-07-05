#include "systick.h"

static volatile unsigned int _dly = 0;

void SysTick_Handler(void) {
    if (_dly != 0)
        _dly--;
}

void systick_init(void)
{
    SysTick_Config(SystemCoreClock/1000);
}

void delay_ms(unsigned int d)
{
    _dly = d;
    while (_dly);
}
