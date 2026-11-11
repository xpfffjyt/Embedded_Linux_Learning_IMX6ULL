# AGENTS.md — 项目上下文（给 AI 助手 / 换设备后快速接手）

本文件用于让 DeepSeek Harness 或其他 AI 助手在**克隆本仓库后立刻获得必要背景**，不必重新侦察板子。

## 项目是什么

一个 MCU 工程师（4 年 STM32 经验）学习嵌入式 Linux 的项目仓库，硬件是野火 **EBF6ULL S1**（NXP i.MX6ULL，Cortex-A7 单核，512MB DDR3，8GB eMMC）。
目标是走完 M0~M7 里程碑，最终做出一个「工业数据采集网关」（自定义驱动 + 多线程应用 + Web/MQTT + systemd）。

## 板子实测事实（2026-11-11 从串口读出，改动前请先复核）

| 项 | 值 |
|---|---|
| 内核 | `4.19.35-imx6`，PREEMPT，**由 gcc 7.5.0 (Linaro) 编译** |
| 系统 | Debian 10 buster，`embedfire.com Debian Image 2025-08-23` |
| 登录 | `debian` / `temppwd`（厂商默认，横幅上印着） |
| sudo | 需要密码（`echo temppwd \| sudo -S ...`） |
| 分区 | `mmcblk1p1` 40M FAT → `/boot`；`mmcblk1p2` 7.2G ext4 → `/`；`mmcblk1boot0/boot1` 4M 存 U-Boot |
| 网口 | **ETH1 / ETH2 两个物理网口，没有 eth0**；另有 `usb0` = 192.168.7.2（g_multi gadget） |
| 调试串口 | **J15 四针：GND / RXD / TXD / VREF**，JST 端子，需外部 3.3V USB-TTL；板上**无** CH340 |
| USB 控制台 | `COM8` = **USB Type-C「USB Device」口**（OTG）的 gadget 虚拟串口（`ttyGS0`），**仅 Linux 起来后存在**，重启会消失 20 秒左右 |
| 其它板载 | 8 位拨码开关可切 NAND/eMMC/SD/USB 启动；micro SD 槽；RTC 电池座（CR1220，未装）；74LV595 扩 8 路 IO |
| 内核控制台 | `console=ttymxc0`（UART1），即 J15 那根线 |
| 设备树 | U-Boot + `/boot/uEnv.txt` + `.dtbo` overlay，`enable_uboot_overlays=1`，40 个可用 dtbo 在 `/usr/lib/linux-image-4.19.35-imx6/overlays/` |
| RGB LED | 被 `leds-gpio`（节点 `normal_led`）占用：红 `GPIO1_IO04`、绿 `GPIO3_IO20`、蓝 `GPIO3_IO19`，全 active-low |
| 其它已占用 GPIO | `cpu` 心跳灯 `GPIO4_IO03`；按键 `Key 0` `GPIO4_IO01`；74HC595 cs/mosi/sck `GPIO4_IO7/10/11` |
| gpiochip | chip0~4 = GPIO1~GPIO5（各 32 线）；chip5 = `74hc595`（8 线） |
| CAN | 默认**未启用**，需在 `uEnv.txt` 打开 `imx-fire-can1.dtbo` |
| 工具链 | 内核由 gcc 7.5.0 编译；模块必须与运行内核匹配，否则 `Invalid module format` |
| 内存 | CMA 预留 320MB，实际可用约 160MB |
| RTC | 无电池，开机时间错乱，联网后需校时 |

## 仓库约定

1. **C 源码注释用英文（ASCII）**：老工具链和串口控制台对 UTF-8 不友好；文档用中文。
2. 驱动是**教学代码**，故意保留简化（单实例、无并发保护）；加分题才是补 `mutex`/`poll`/`sysfs`/`input`。
3. **不要提交编译产物**：`.ko`/`.o`/`.dtbo`/可执行文件已在 `.gitignore` 中。
4. **不要猜引脚**：`GPIO1_IO04` 是实测值；换板子必须重跑 `gpioinfo | grep used` 核对。
5. 改 `/boot/uEnv.txt` 前务必 `cp` 备份 —— 那是启动配置，写坏了板子起不来（救援只能靠 J15 UART）。

## 关键路径

- 主教程：`imx6ull-linux-roadmap.md`（含 M0~M7 详细步骤、调试方法论、23 条踩坑、20 道自测题）
- 配套代码：`imx6ull-bootcamp/`（M0 hello、M3 模块、M4 LED 驱动 + overlay）
- Windows 工具：`imx6ull-bootcamp/tools/serial-log.ps1`
- Ubuntu 环境：`imx6ull-bootcamp/tools/setup-ubuntu.sh`

## 当前进度

已完成：环境侦察、路线图、配套代码骨架、Ubuntu 配置脚本。
待办：VMware + Ubuntu 22.04 构建机 → M0 闭环 → 等 USB-TTL 线到货后做 M1（打断 U-Boot）。

> 协助用户时：**优先保证每一步都能闭环验收**，不要一次给出跨越多个里程碑的任务；涉及板子状态变更（rebooot、改 uEnv.txt、insmod）时先备份并说明可回退方式。
