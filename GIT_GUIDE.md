# Git 上传操作说明

## 我帮你做了什么

### 1. 运行 `keilkill.bat` 清理编译产物

删除了 `Objects/` 和 `Listings/` 目录下的编译中间文件：
- `.o` / `.d` / `.crf` / `.axf` / `.htm` / `.sct` / `.lst` / `.map` / `.dep` / `.lnp`

**为什么**：这些文件是编译器自动生成的，每个人的电脑上编译都会重新产生。上传这些文件没有意义，还会让仓库体积变大、产生无意义的冲突。

---

### 2. 创建 `.gitignore` 文件

这个文件告诉 Git **哪些文件不要追踪、不要上传**。

排除了：
- 所有 Keil 编译中间文件（`*.o`, `*.d`, `*.crf` 等）
- 用户特定的 Keil 配置文件（`*.uvguix.*`）——这些文件包含你电脑上的绝对路径和用户名，泄露到网上有隐私风险
- DebugConfig 调试配置目录
- JLink 日志

**为什么重要**：

| 没有 .gitignore | 有 .gitignore |
|---|---|
| 编译产物、用户路径全上传 | 只上传源代码和工程配置 |
| 换台电脑拉下来编译就冲突 | 换台电脑拉下来直接编译 |
| 你的 Windows 用户名暴露在互联网上 | 隐私得到保护 |

---

### 3. 创建 `README.md`

GitHub 仓库首页的说明文档。包含：
- 项目介绍
- 硬件平台和接线
- 功能说明和操作指南
- 项目目录结构
- 关键技术点说明

---

### 4. 初始化 Git 仓库并推送到 GitHub

执行了以下操作：

```bash
git init                          # 在本目录创建 Git 仓库
git remote add origin <你的URL>    # 关联 GitHub 远程仓库
git add -A                        # 暂存所有文件（.gitignore 会自动排除不应上传的）
git commit -m "Initial commit"    # 创建第一次提交（快照）
git push -u origin master         # 推送到 GitHub
```

推送到了：`https://github.com/baicaixiao2025/SObK-EC`

---

## Git 基本概念（给第一次使用的人）

### 三个关键概念

| 概念 | 通俗理解 |
|------|---------|
| **commit（提交）** | 给当前所有文件拍一张"快照"，可以随时回到这个状态 |
| **push（推送）** | 把本地的快照上传到 GitHub |
| **pull（拉取）** | 从 GitHub 下载别人的更新 |

### 你以后的工作流程

```bash
# 1. 改了代码之后
git add -A                        # 暂存修改
git commit -m "描述你改了什么"     # 拍快照

# 2. 想上传到 GitHub 的时候
git push                          # 推送到 GitHub

# 3. 如果在另一台电脑上开发，先拉取最新代码
git pull                          # 从 GitHub 拉取
```

---

## 已推送到 GitHub 的内容（86 个文件）

```
.gitignore              ← Git 忽略规则
README.md               ← 项目说明
CHANGELOG.md            ← 详细修改记录
keilkill.bat            ← 清理脚本
Project.uvprojx         ← Keil 工程文件
Project.uvoptx          ← Keil 工程选项
Hardware/               ← 外设驱动（编码器、按键、OLED、舵机、PWM）
Library/                ← STM32F10x 标准外设库
Start/                  ← 启动文件 + CMSIS
System/                 ← 系统延时
User/                   ← 主程序
Objects/.gitkeep        ← 空目录占位（编译时在此生成 .o 文件）
Listings/.gitkeep       ← 空目录占位（编译时在此生成 .lst 文件）
```

## 没有上传的内容（被 .gitignore 排除）

```
Project.uvguix.Admin    ← 另一台电脑的 Keil 用户配置
Project.uvguix.XD-63    ← 你的 Keil 用户配置（含 Windows 用户名路径）
DebugConfig/            ← 调试配置
所有 .o .d .crf .axf    ← 编译中间产物
```

---

## 注意事项

1. **仓库目前是公开的（public）**——任何能上网的人都能看到代码。如果希望私有，去 GitHub 仓库 Settings → Danger Zone → Change visibility。
2. **Keil 工程文件** `Project.uvprojx` 和 `Project.uvoptx` 已上传，团队成员用 Keil 打开即可编译。
3. **每次编译后**建议运行 `keilkill.bat` 再提交，避免把编译产物混进去。
4. **以后改代码时**，记得写完一段就 `commit`，避免积累太多改动后忘记改了什么。
