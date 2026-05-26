#include "stm32f10x.h"

/**
 * @brief  初始化按键 GPIO
 *         Key1: PA4 (上拉输入)
 *         Key2: PB11 (上拉输入)
 */
void Key_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
}

/**
 * @brief  非阻塞按键扫描（边沿检测）
 * @retval 0: 无按键, 1: Key1 按下, 2: Key2 按下
 */
uint8_t Key_GetNum(void)
{
    static uint8_t prev_key1 = 1;
    static uint8_t prev_key2 = 1;
    uint8_t KeyNum = 0;
    uint8_t cur1, cur2;

    cur1 = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_4);
    if (cur1 == 0 && prev_key1 == 1)
        KeyNum = 1;
    prev_key1 = cur1;

    cur2 = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_11);
    if (cur2 == 0 && prev_key2 == 1)
        KeyNum = 2;
    prev_key2 = cur2;

    return KeyNum;
}
