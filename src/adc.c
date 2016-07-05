#include "stm32f0xx_conf.h"
#include "stm32f0xx_adc.h"

uint8_t trimmer_value;

void ADC1_COMP_IRQHandler(void)
{
	if (ADC_GetITStatus(ADC1, ADC_IT_EOC) == SET) {
		trimmer_value = ADC_GetConversionValue(ADC1);
		ADC_ClearITPendingBit(ADC1, ADC_IT_EOC);
	}
}

void adc_init(void)
{
	GPIO_InitTypeDef gi;
	ADC_InitTypeDef ai;
	NVIC_InitTypeDef ni;
	TIM_TimeBaseInitTypeDef ti;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);

	gi.GPIO_Mode = GPIO_Mode_AN;
	gi.GPIO_Pin = GPIO_Pin_0;
	gi.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &gi);

	ai.ADC_ContinuousConvMode = ENABLE;
	ai.ADC_Resolution = ADC_Resolution_8b;
	ai.ADC_DataAlign = ADC_DataAlign_Right;
	ai.ADC_ScanDirection = ADC_ScanDirection_Upward;
	ai.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
	ai.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T3_TRGO;

	ADC_Init(ADC1, &ai);
	ADC_Cmd(ADC1, ENABLE);

	ni.NVIC_IRQChannel = ADC1_COMP_IRQn;
	ni.NVIC_IRQChannelPriority = 1;
	ni.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&ni);

	ADC_ITConfig(ADC1, ADC_IT_EOC, ENABLE);

	ADC_ChannelConfig(ADC1, ADC_Channel_0, ADC_SampleTime_239_5Cycles);
	ADC_StartOfConversion(ADC1);

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

	// 5000 counts in 20ms -> 4us
	ti.TIM_CounterMode = TIM_CounterMode_Up;
	ti.TIM_ClockDivision = TIM_CKD_DIV4; // 6Mhz
	ti.TIM_Prescaler = 6*1000; // 1kHz
	ti.TIM_Period = 200; // 5Hz

	TIM_TimeBaseInit(TIM3, &ti);
/*
	ni.NVIC_IRQChannel = TIM3_IRQn;
	ni.NVIC_IRQChannelPriority = 1;
	ni.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&ni);

	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
	*/
	TIM_Cmd(TIM3, ENABLE);
}
