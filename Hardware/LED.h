#ifndef __LED_H
#define __LED_H




void LED_Init(void);
void LED1_ON(void);
void LED1_OFF(void);
void LED2_ON(void);
void LED2_OFF(void);
void LED_SET_ON(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
void LED_SET_OFF(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
void LED1_Turn(void);
void LED2_Turn(void);

#endif
