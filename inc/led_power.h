#ifndef LED_POWER_H_
#define LED_POWER_H_

void led_power_init(int init_irq);
void led_power_enable(int on);
int led_power_check_and_clear_fault(void);

#endif // LED_POWER_H_
