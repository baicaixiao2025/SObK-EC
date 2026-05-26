/**
 * @file    main.c
 * @brief   PWM 舵机控制程序 —— 按键 + 旋转编码器双控
 *
 * 控制方式：
 *   - Key2 (PB11): 角度跳变到下一个预设档位（0→30→60→90→120→150→180→0 循环）
 *   - Key1 (PA4):  角度跳变到上一个预设档位（反向循环）
 *   - 旋转编码器:   微调角度，步长 ±1°/脉冲
 *
 * 优先级：按键 > 编码器（按键触发时丢弃本轮编码器累积量）
 *
 * 硬件平台: STM32F103C8
 * @note   编码器：PB0/PB1，Key1：PA4，Key2：PB11，舵机 PWM：PA1，OLED：PB8/PB9。
 */

#include "stm32f10x.h"
#include "OLED.h"
#include "Servo.h"
#include "Key.h"
#include "Encoder.h"

/* 预设档位表（单位：度），按键跳变使用 */
static const uint8_t AnglePresets[] = {0, 30, 60, 90, 120, 150, 180};
static const uint8_t PresetCount = sizeof(AnglePresets) / sizeof(AnglePresets[0]);

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
        uint8_t key;
        int16_t enc_delta;

        /* ---- 第 1 步：读取输入设备 ---- */
        key       = Key_GetNum();
        enc_delta = Encoder_Get();

        /* ---- 第 2 步：按键处理（优先级高于编码器）---- */
        if (key == 2) /* Key2 (PB11): 跳到下一个更高预设档位 */
        {
            uint8_t i;
            for (i = 0; i < PresetCount; i++)
            {
                if (AnglePresets[i] > Angle)
                {
                    Angle = (float)AnglePresets[i];
                    break;
                }
            }
            if (i == PresetCount) /* 已超过最大档位，回绕到 0° */
            {
                Angle = 0.0f;
            }
            enc_delta = 0; /* 丢弃本轮编码器累积量 */
        }
        else if (key == 1) /* Key1 (PB1): 跳到下一个更低预设档位 */
        {
            int8_t i;
            for (i = (int8_t)(PresetCount - 1); i >= 0; i--)
            {
                if (AnglePresets[i] < Angle)
                {
                    Angle = (float)AnglePresets[i];
                    break;
                }
            }
            if (i < 0) /* 已低于最小档位，回绕到 180° */
            {
                Angle = 180.0f;
            }
            enc_delta = 0;
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
