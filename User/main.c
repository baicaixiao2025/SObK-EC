/**
 * @file    main.c
 * @brief   PWM 舵机控制程序 —— B11 单按键循环 + 旋转编码器微调
 *
 * 控制方式：
 *   - B11 (PB11): 单按键循环跳变 30→60→90→120→150→180→150→120→90→60→30→...
 *   - 旋转编码器: 微调角度，步长 ±1°/脉冲
 *
 * 优先级：按键 > 编码器（按键触发时丢弃本轮编码器累积量）
 *
 * 硬件平台: STM32F103C8
 * @note   编码器：PB0/PB1，按键：PB11，舵机 PWM：PA1，OLED：PB8/PB9。
 */

#include "stm32f10x.h"
#include "OLED.h"
#include "Servo.h"
#include "Key.h"
#include "Encoder.h"

/* 循环跳变序列（单位：度），30→180 来回 */
static const uint8_t AngleCycle[] = {0, 30, 60, 90, 120, 150, 180, 150, 120, 90, 60, 30};
static const uint8_t CycleLen = sizeof(AngleCycle) / sizeof(AngleCycle[0]);

int main(void)
{
    /* 外设初始化 */
    OLED_Init();
    Servo_Init();
    Key_Init();
    Encoder_Init();

    /* OLED 静态标签 */
    OLED_ShowString(1, 1, "Angle:");
    OLED_ShowString(2, 1, "Encoder:");

    /* 初始角度：90°（舵机中位） */
    float Angle = 90.0f;
    Servo_setAngle(Angle);
    OLED_ShowNum(1, 7, (uint32_t)Angle, 3);
    OLED_ShowNum(2, 9, 0, 4);

    while (1)
    {
        static uint8_t cycleIdx = 3; /* AngleCycle[3] = 90, 初始中位 */
        uint8_t key;
        int16_t enc_delta;

        /* ---- 第 1 步：读取输入设备 ---- */
        key       = Key_GetNum();
        enc_delta = Encoder_Get();

        /* ---- 第 2 步：按键处理（优先级高于编码器）---- */
        if (key == 2) /* B11: 循环跳变到下一个档位 */
        {
            cycleIdx = (cycleIdx + 1) % CycleLen;
            Angle = (float)AngleCycle[cycleIdx];
            enc_delta = 0; /* 丢弃本轮编码器累积量 */
        }
        /* ---- 第 3 步：编码器微调（无按键时生效）---- */
        else if (enc_delta != 0)
        {
            Angle += (float)enc_delta * 1.0f;

            /* 角度钳位 */
            if (Angle < 0.0f)   Angle = 0.0f;
            if (Angle > 180.0f) Angle = 180.0f;
        }

        /* ---- 第 4 步：更新 PWM 和 OLED ---- */
        Servo_setAngle(Angle);
        OLED_ShowNum(1, 7, (uint32_t)Angle, 3);
        OLED_ShowNum(2, 9, (uint32_t)(enc_delta >= 0 ? enc_delta : -enc_delta), 4);
    }
}
