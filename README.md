# 舵机 PWM 测试控制程序

基于 STM32F103C8 的舵机 PWM 测试小程序，用于生成 PWM 信号驱动舵机并验证角度控制。支持**按键档位跳变**与**旋转编码器微调**双控模式。

## 硬件平台

- MCU：STM32F103C8（ARM Cortex-M3，72MHz）
- IDE：Keil MDK（ARM Compiler）
- 显示：OLED（I2C，PB8/PB9）
- 编码器：旋转编码器（PB0 A 相 / PB1 B 相）
- 按键：Key1 (PA4)、Key2 (PB11)
- 舵机：PWM 输出 (PA1)

## 功能

| 操作 | 效果 |
|------|------|
| 按 **Key2** (PB11) | 角度跳到下一个高档位：0→30→60→90→120→150→180→0… |
| 按 **Key1** (PA4) | 角度跳到下一个低档位：…180→150→120→90→60→30→0→180… |
| 顺时针旋转编码器 | 角度 +1°/脉冲 |
| 逆时针旋转编码器 | 角度 -1°/脉冲 |
| 按键后转编码器 | 从按键档位出发微调 |

**优先级**：按键 > 编码器（按键触发时丢弃本轮编码器累积量）

## 项目结构

```
├── Hardware/          # 外设驱动层
│   ├── Encoder.c/h    # 旋转编码器（EXTI 中断 + DWT 消抖）
│   ├── Key.c/h        # 按键（非阻塞边沿检测）
│   ├── LED.c/h        # LED 指示
│   ├── OLED.c/h       # OLED 显示（I2C）
│   ├── PWM.c/h        # PWM 底层驱动（TIM2）
│   └── Servo.c/h      # 舵机角度控制
├── Library/           # STM32F10x 标准外设库 (v1.30)
├── Start/             # 启动文件 + CMSIS Core
├── System/            # 系统滴答延时 (Delay)
├── User/              # 主程序 + 中断服务
├── Project.uvprojx    # Keil 工程文件
└── CHANGELOG.md       # 详细修改记录
```

## 关键技术点

- **DWT 消抖**：利用 ARM Cortex-M3 内建周期计数器，2ms 窗口滤除编码器触点弹跳，不占用 TIM/SysTick
- **竞态修复**：`Encoder_Get()` 内关全局中断实现原子读-清零，防止 ISR 计数丢失
- **非阻塞按键**：边沿检测替代阻塞等待，主循环无冻结
- **GPIO 冲突缓解**：PB1 被 Key1 和编码器 B 相共享，通过 Suspend/Resume 隔离干扰

## 编码器消抖说明

机械编码器存在触点弹跳，本程序使用 DWT 周期计数器实现消抖：

- 两次有效边沿间隔需 ≥ 2ms（约 150,000 个 CPU 周期）
- 无需额外定时器外设
- 无符号减法自动处理计数器溢出回绕

## 已知限制

PB1 引脚同时连接 Key1 和编码器 B 通道，存在硬件冲突。软件通过 `Encoder_Suspend()`/`Encoder_Resume()` 缓解——读取 Key1 时暂停 EXTI1 中断。按住 Key1 不放时编码器方向判断可能出错。**硬件根治方案**：将 Key1 改焊到其他空闲 GPIO（如 PA0、PB12、PB13）。

## 开发环境

- IDE: Keil MDK μVision
- 编译器: ARM Compiler（默认优化等级）
- 依赖: STM32F10x 标准外设库（源码已包含在 `Library/` 下）

## 烧录与调试

使用 ST-Link / J-Link 通过 SWD 接口连接：

| 信号 | 引脚 |
|------|------|
| SWCLK | PA14 |
| SWDIO | PA13 |
| GND | GND |
| 3.3V | 3V3 |

## 许可证

This project is for educational and personal use.
