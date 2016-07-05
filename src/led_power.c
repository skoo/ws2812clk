#include "stm32f0xx_conf.h"
#include "led_power.h"

static int _led_power_fault = 0;

void EXTI0_1_IRQHandler(void)
{
	if (EXTI_GetITStatus(EXTI_Line0) == SET)
	{
		led_power_enable(0);
		EXTI_ClearITPendingBit(EXTI_Line0);
		_led_power_fault = 1;
	}
}

int led_power_check_and_clear_fault(void)
{
	int fault = _led_power_fault;
	_led_power_fault = 0;
	return fault;
}

void led_power_enable(int on)
{
	if (on)
		GPIO_ResetBits(GPIOF, GPIO_Pin_0);
	else
		GPIO_SetBits(GPIOF, GPIO_Pin_0);
}

void led_power_init(int init_irq)
{
	EXTI_InitTypeDef ei;
	GPIO_InitTypeDef gi;
	NVIC_InitTypeDef ni;

	gi.GPIO_Pin = GPIO_Pin_0;
	gi.GPIO_Mode = GPIO_Mode_OUT;
	gi.GPIO_OType = GPIO_OType_PP;
	gi.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOF, &gi); // PF0: LED voltage enable

	gi.GPIO_Pin = GPIO_Pin_1;
	gi.GPIO_Mode = GPIO_Mode_IN;
	GPIO_Init(GPIOF, &gi); // PF1: LED fault

	if (init_irq)
	{
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
        SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOF, EXTI_PinSource1);

        ei.EXTI_Mode = EXTI_Mode_Interrupt;
        ei.EXTI_Line = EXTI_Line0;
        ei.EXTI_Trigger = EXTI_Trigger_Falling;
        ei.EXTI_LineCmd = ENABLE;
        EXTI_Init(&ei);

        ni.NVIC_IRQChannel = EXTI0_1_IRQn;
        ni.NVIC_IRQChannelPriority = 1;
        ni.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&ni);
	}
}
