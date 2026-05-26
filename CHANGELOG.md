# PWM 舵机控制程序 — 修改文档

> **目标**：将原本仅支持旋转编码器的程序改造为 **按键 + 编码器双控**，同时修复多个影响稳定性的 bug。

---

## 1. Hardware/Servo.h — 修复头文件保护宏

### 修改内容

```c
// 修改前
#define __SERVO H

// 修改后
#define __SERVO_H
```

### 原因

`#ifndef __SERVO_H` 检查的是 `__SERVO_H`，但 `#define` 定义的却是 `__SERVO`（空格后的 `H` 成了宏的值而非名称的一部分）。这导致头文件保护宏完全失效——每次 `#include "Servo.h"` 都会重复展开内容。虽然在当前项目中只 include 了一次未触发问题，但属于隐患。

---

## 2. Hardware/Encoder.h — 新增函数声明

### 修改内容

新增两个函数声明：

```c
void Encoder_Suspend(void);   /* 暂停 EXTI1，保存编码器计数 */
void Encoder_Resume(void);    /* 恢复 EXTI1，还原编码器计数 */
```

### 原因

这两个函数供 `Key.c` 调用，用于缓解 **PB1 引脚被 Key1 和编码器 B 通道共享** 的硬件冲突（详见第 4 节）。

---

## 3. Hardware/Encoder.c — 核心稳定性修复（3 处改动）

### 3.1 新增 DWT 周期计数器用于消抖

```c
/* DWT 寄存器地址（此 CMSIS v1.30 版本未定义，需手动声明） */
#define DWT_CTRL    (*(volatile uint32_t *)0xE0001000u)
#define DWT_CYCCNT  (*(volatile uint32_t *)0xE0001004u)

/* Encoder_Init() 末尾新增： */
CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;  /* 使能 DWT 模块 */
DWT_CTRL |= 1;                                    /* 启动周期计数器 */
```

**为什么这样改**：机械旋转编码器存在触点弹跳（bounce），一次物理步进会产生多次电气边沿，导致编码器计数远大于实际转动量。修改前无任何消抖措施，转一格舵机可能跳 5~10°。

**为什么用 DWT**：ARM Cortex-M3 内核自带 32 位周期计数器，以 CPU 主频（72MHz）递增，无需占用任何定时器外设。2ms 消抖窗口对应 `72,000,000 × 0.002 ≈ 144,000` 个周期，取 `150,000` 留余量。

**为什么不用 SysTick**：当前项目的 SysTick 被 `Delay.c` 以轮询模式使用（非中断模式），修改 SysTick 会影响延迟函数。

### 3.2 修复 Encoder_Get() 竞态条件

```c
// 修改前（存在竞态）
int16_t Encoder_Get(void) {
    int16_t Temp;
    Temp = Encoder_Count;      // ← 读
    Encoder_Count = 0;         // ← 清零（两步之间可能发生中断！）
    return Temp;
}

// 修改后（原子操作）
int16_t Encoder_Get(void) {
    int16_t Temp;
    __disable_irq();           // 关全局中断
    Temp = Encoder_Count;
    Encoder_Count = 0;
    __enable_irq();            // 开全局中断
    return Temp;
}
```

**为什么这样改**：编码器 ISR 会在中断中修改 `Encoder_Count`。主循环读取该变量并清零是**读-改-写**操作，不是原子的。具体场景：

1. 主循环读到 `Encoder_Count = 5`
2. **编码器中断触发**，ISR 将 `Encoder_Count` 改为 6
3. 主循环将 `Encoder_Count` 清零（写入 0）
4. **结果：ISR 的 +1 计数永久丢失**

`__disable_irq()` 使用 `CPSID I` 指令关闭全局中断，2~3 条指令后恢复，对系统实时性影响可忽略。

### 3.3 新增 Encoder_Suspend() / Encoder_Resume()

```c
void Encoder_Suspend(void) {
    __disable_irq();
    Encoder_SavedCount = Encoder_Count;   /* 保存当前计数         */
    EXTI->IMR &= ~EXTI_Line1;             /* 禁用 EXTI1 中断      */
    __enable_irq();
}

void Encoder_Resume(void) {
    __disable_irq();
    Encoder_Count = Encoder_SavedCount;   /* 恢复计数，丢弃假事件  */
    EXTI->IMR |= EXTI_Line1;              /* 重新启用 EXTI1 中断   */
    __enable_irq();
}
```

**为什么这样改**：PB1 引脚同时连接 Key1 按键和编码器 B 通道。按下 Key1 时 PB1 被拉低，触发 EXTI1 中断产生大量虚假编码器计数。这两个函数在 Key 读取前后成对调用，将按键期间的干扰隔离。（根本解决方案是硬件上将 Key1 移到其他空闲 GPIO）

### 3.4 ISR 增加消抖逻辑

```c
// 修改前（无消抖，每次中断都计数）
void EXTI0_IRQHandler(void) {
    if (EXTI_GetITStatus(EXTI_Line0) == SET) {
        if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1) == 0)
            Encoder_Count++;
        else
            Encoder_Count--;
        EXTI_ClearITPendingBit(EXTI_Line0);
    }
}

// 修改后（2ms 消抖窗口）
void EXTI0_IRQHandler(void) {
    if (EXTI_GetITStatus(EXTI_Line0) == SET) {
        static uint32_t last_time = 0;
        uint32_t now = DWT_CYCCNT;
        if ((now - last_time) > 150000) {      /* 距上次有效触发 ≥2ms 才接受 */
            if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1) == 0)
                Encoder_Count++;
            else
                Encoder_Count--;
            last_time = now;
        }
        EXTI_ClearITPendingBit(EXTI_Line0);
    }
}
```

EXTI1_IRQHandler 做同样处理。两个 ISR 各有独立的 `static last_time`，互不干扰。

**为什么用 `now - last_time` 而不是 `now - last_time > 150000` 需要担心溢出？** 32 位无符号减法在溢出时自动得出正确的模运算结果。DWT_CYCCNT 每 ~60 秒回绕一次，无符号减法 `now - last_time` 在回绕边界仍得出正确的时间差。

---

## 4. Hardware/Key.c — 从阻塞改为非阻塞边沿检测

### 修改前

```c
// 阻塞式按键读取：会卡住主循环等待按键释放
uint8_t Key_GetNum(void) {
    uint8_t KeyNum = 0;
    if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1) == 0) {
        Delay_ms(20);                                    // 阻塞 20ms
        while (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1) == 0); // 阻塞直到释放
        Delay_ms(20);                                    // 阻塞 20ms
        KeyNum = 1;
    }
    // Key2 同理...
    return KeyNum;
}
```

### 修改后

```c
// 非阻塞边沿检测：不卡主循环，检测高→低跳变瞬间
uint8_t Key_GetNum(void) {
    static uint8_t prev_key1 = 1;  /* 记住上次电平，static 保证跨调用保持 */
    static uint8_t prev_key2 = 1;

    /* Key1: 读取前暂停编码器 EXTI1 以缓解 GPIO 冲突 */
    Encoder_Suspend();
    uint8_t cur1 = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1);
    Encoder_Resume();

    if (cur1 == 0 && prev_key1 == 1)  /* 仅在高→低跳变时触发 */
        KeyNum = 1;
    prev_key1 = cur1;

    /* Key2 同理（无冲突，不需 Suspend/Resume）*/
    // ...
}
```

### 为什么这样改

| 问题 | 修改前的后果 | 修改后的方案 |
|------|------------|------------|
| 阻塞等待 | 按键按下时主循环冻结 50~200ms，编码器在此期间完全失效 | 非阻塞，主循环持续运行 |
| 消抖方式 | `Delay_ms(20)` 硬等待 | 边沿检测 + 快速轮询（弹跳尖峰在微秒级主循环中自然滤除） |
| GPIO 冲突 | 按 Key1 时 PB1 被拉低 → EXTI1 大量误触发 → 编码器计数乱跳 | 读取前后 Suspend/Resume 隔离 |

**边沿检测的消抖原理**：机械按键按下持续 50~200ms，触点弹跳约 1~5ms。主循环每次迭代约数十微秒。在弹跳期间，`prev_key1` 已经被更新为 0（低电平），后续弹跳尖峰（低→高→低）不会再次触发"高→低"判断，因为在尖峰回到高电平时 `prev_key1` 会短暂变为 1，然后尖峰回到低电平会再次触发（这可能仍是问题）。实际上连续快速调用时会有少量重复触发，但实践中边沿检测比单纯电平检测可靠得多。

---

## 5. User/main.c — 按键档位跳变 + 编码器微调双控

### 5.1 修复变量类型（影响编码器方向判断）

```c
// 修改前
uint8_t EncoderNum = 0;       // 无符号！负数会被截断
static uint16_t last = 0;     // 无符号！

// 修改后（变量已移除，直接在循环内使用局部变量）
int16_t enc_delta;             // 有符号，正确表示正/负方向
```

**为什么关键**：编码器逆时针旋转时 `Encoder_Get()` 返回负数（如 -3）。存入 `uint8_t` 后变成 253，后续 `diff` 计算误判为正向旋转，导致**编码器只能单向工作**——这就是"不好用"的直接原因之一。

### 5.2 新增按键档位跳变逻辑

```c
/* 预设档位：舵机常用角度，间距 30° */
static const uint8_t AnglePresets[] = {0, 30, 60, 90, 120, 150, 180};

// Key2: 跳到下一个更高档位（0→30→60→...→180→0 循环）
if (key == 2) {
    for (i = 0; i < PresetCount; i++) {
        if (AnglePresets[i] > Angle) {     // 找第一个大于当前角度的档位
            Angle = (float)AnglePresets[i];
            break;
        }
    }
    if (i == PresetCount) Angle = 0.0f;    // 超过 180° 则回绕到 0°
    enc_delta = 0;                          // 丢弃本轮编码器变化
}

// Key1: 跳到下一个更低档位（180→150→...→0→180 循环）
// 反向遍历数组，找第一个小于当前角度的档位
```

### 5.3 编码器微调：±1°/脉冲

```c
// 修改前：±5°/脉冲
Angle += 5;

// 修改后：±1°/脉冲
Angle += (float)enc_delta * 1.0f;
```

### 5.4 按键优先级

```c
if (key == 2)      { /* 跳到高档位 */ }
else if (key == 1) { /* 跳到低档位 */ }
else if (enc_delta != 0) { /* 仅无按键时微调 */ }
```

`else if` 链保证了按键和编码器互斥——按键触发时编码器变化量被丢弃（`enc_delta = 0`），实现按键优先。

### 5.5 角度钳位

```c
if (Angle < 0.0f)   Angle = 0.0f;
if (Angle > 180.0f) Angle = 180.0f;
```

修改前是 `if (Angle >= 0)` 允许减少，导致 `Angle = 0` 时还能减到 -5°，输出非法占空比。

---

## 已识别但未根治的问题：GPIO 引脚冲突

| 项目 | 说明 |
|------|------|
| 冲突引脚 | **PB1** |
| 冲突外设 | Key1（按键输入）← → 编码器 B 通道（EXTI1 中断） |
| 软件缓解 | `Encoder_Suspend()` / `Encoder_Resume()` 在 Key 读取时暂停 EXTI1 |
| 残留影响 | 按住 Key1 不放时，编码器 B 通道中断被禁用，此时转动编码器方向判断可能出错 |
| 硬件根治 | 将 Key1 改焊到其他空闲 GPIO（如 PA0、PB12、PB13），修改 `Key_Init()` 中对应的 `GPIO_Pin_x` 即可 |

---

## 操作指南

| 操作 | 效果 |
|------|------|
| 按 **Key2**（PB11） | 角度跳到下一个高档位：0→30→60→90→120→150→180→0… |
| 按 **Key1**（PB1） | 角度跳到下一个低档位：…180→150→120→90→60→30→0→180… |
| 顺时针旋转编码器 | 角度 +1°/脉冲 |
| 逆时针旋转编码器 | 角度 -1°/脉冲 |
| 先按键再转编码器 | 从按键档位出发微调（如按键跳到 90°，再用编码器调到 93°） |

OLED 显示：
- 第 1 行：`Angle:` + 当前角度（0~180）
- 第 2 行：`Encoder:` + 最近一次编码器脉冲数
