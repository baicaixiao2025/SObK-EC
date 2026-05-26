#include "stm32f10x.h"

/* DWT (Data Watchpoint and Trace) 周期计数器寄存器
   此 CMSIS v1.30 版本未定义 DWT->CYCCNT，需手动定义地址 */
#define DWT_CTRL    (*(volatile uint32_t *)0xE0001000u)
#define DWT_CYCCNT  (*(volatile uint32_t *)0xE0001004u)

volatile int16_t Encoder_Count;

/**
 * @brief  初始化旋转编码器
 *         通道 A: PB0 (EXTI0), 通道 B: PB1 (EXTI1)
 *         下降沿触发，内部上拉
 */
void Encoder_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    /* GPIO 配置：PB0、PB1 作为上拉输入 */
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* 映射 GPIO 到 EXTI 线 */
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource0);
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource1);

    /* EXTI 配置：下降沿中断 */
    EXTI_InitTypeDef EXTI_InitStructure;
    EXTI_InitStructure.EXTI_Line = EXTI_Line0 | EXTI_Line1;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_Init(&EXTI_InitStructure);

    /* NVIC 优先级分组 */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    /* EXTI0 中断优先级 */
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_Init(&NVIC_InitStructure);

    /* EXTI1 中断优先级 */
    NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_Init(&NVIC_InitStructure);

    /* 启用 DWT 周期计数器（用于 ISR 消抖计时，72MHz 下每周期 ≈13.9ns） */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT_CTRL |= 1;
}

/**
 * @brief  读取编码器累积计数并清零
 * @retval 自上次读取以来的净脉冲数（正=顺时针，负=逆时针）
 * @note   关全局中断保护读-清零操作的原子性，防止与 ISR 竞争
 */
int16_t Encoder_Get(void)
{
    int16_t Temp;
    __disable_irq();
    Temp = Encoder_Count;
    Encoder_Count = 0;
    __enable_irq();
    return Temp;
}

/**
 * @brief  EXTI0 中断服务函数（编码器 A 通道下降沿）
 *         含 2ms 消抖：忽略 2ms 内的重复触发，消除机械触点抖动
 */
void EXTI0_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line0) == SET)
    {
        static uint32_t last_time = 0;
        uint32_t now = DWT_CYCCNT;

        /* 2ms 消抖窗口：72MHz * 0.002 ≈ 144000，取 150000 留余量 */
        if ((now - last_time) > 150000)
        {
            /* 正交解码：A 下降沿时读 B 电平判断方向 */
            if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1) == 0)
                Encoder_Count++;
            else
                Encoder_Count--;
            last_time = now;
        }
        EXTI_ClearITPendingBit(EXTI_Line0);
    }
}

/**
 * @brief  EXTI1 中断服务函数（编码器 B 通道下降沿）
 *         含 2ms 消抖，与 EXTI0 对称的正交解码逻辑
 */
void EXTI1_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line1) == SET)
    {
        static uint32_t last_time = 0;
        uint32_t now = DWT_CYCCNT;

        if ((now - last_time) > 150000)
        {
            /* 正交解码：B 下降沿时读 A 电平判断方向 */
            if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_0) == 0)
                Encoder_Count--;
            else
                Encoder_Count++;
            last_time = now;
        }
        EXTI_ClearITPendingBit(EXTI_Line1);
    }
}
