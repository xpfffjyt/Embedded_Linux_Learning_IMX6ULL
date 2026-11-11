# Embedded Linux Learning · i.MX6ULL

一个**从 MCU 工程师视角**出发的嵌入式 Linux 入门工程。

基于野火 **i.MX6ULL（EBF6ULL S1）** 开发板，用「项目驱动」的方式，把 4 年 STM32 裸机/RTOS 经验迁移到 Linux 的驱动开发、应用开发和系统集成上。

> 核心原则：**每 3~5 天必须闭环一次 —— 写代码 → 编译 → 部署到板子 → 看到现象。**
> 任何超过一周没有「可见结果」的学习内容，都往后放。

---

## 这个仓库里有什么

| 内容 | 说明 |
|---|---|
| **路线图** | 8 周 / M0~M7 里程碑，每个都有明确产出物和验收命令 |
| **起步代码** | 第一个交叉编译程序、第一个内核模块、LED 字符驱动 + 设备树 overlay |
| **实测档案** | 板子上每条命令的真实输出 —— 不是抄来的通用描述 |
| **踩坑清单** | 23 条高频坑（现象 / 原因 / 解法），带 PowerShell 执行策略、`-EBUSY` 这类真坑 |
| **工具脚本** | Windows 串口日志抓取、Ubuntu 构建机一键配置 |

---

## 硬件与环境（实测，非猜测）

| 项目 | 值 |
|---|---|
| 开发板 | 野火 EBF6ULL S1（NXP i.MX6ULL，Cortex-A7 单核） |
| 内核 | `4.19.35-imx6`（`PREEMPT`，gcc 7.5.0 编译） |
| 系统 | Debian 10 buster，`embedfire.com Debian Image 2025-08-23` |
| 登录 | `debian` / `temppwd`（厂商默认，登录横幅上直接印着） |
| 存储 | `mmcblk1p1` 40M FAT → `/boot`；`mmcblk1p2` 7.2G ext4 → `/` |
| 网络 | ETH1 / ETH2 双网口（**没有 eth0**），另有 `usb0` gadget 网卡 |
| 调试串口 | **J15 四针（GND / RXD / TXD / VREF）**，需外接 3.3V USB-TTL |
| USB 控制台 | `COM8` = **USB Type-C「USB Device」口**（OTG）虚拟出的 gadget 串口（`ttyGS0`），**Linux 起来后才有**，重启会消失 20 秒左右 |
| 设备树机制 | U-Boot + `/boot/uEnv.txt` + `.dtbo` overlay（**不是**重编整份 dts） |
| RGB LED | 已被 `leds-gpio` 占用：红 `GPIO1_IO04`、绿 `GPIO3_IO20`、蓝 `GPIO3_IO19` |

⚠️ 所以：**看 U-Boot 必须接 J15 那根线**，COM8 永远只能看到内核起来之后的世界。

---

## 目录结构

```
Embedded_Linux_Learning_IMX6ULL/
├── imx6ull-linux-roadmap.md        主教程（路线图 + 调试方法论 + 高频坑 + 20 道自测题）
├── images/
│   └── board-j15-uart-pinout.png   J15 调试串口丝印特写（接线前必看）
└── imx6ull-bootcamp/
    ├── README.md                   配套代码说明与分步操作
    ├── m0-hello/                   第一个交叉编译程序（含 make check / static）
    ├── m3-hello-module/            第一个内核模块（含模块参数）
    ├── m4-led-driver/              LED 字符驱动 + overlay-ebf-led.dts + 测试程序
    └── tools/
        ├── serial-log.ps1          Windows 串口探针 / 启动日志抓取
        └── setup-ubuntu.sh         Ubuntu 22.04 构建机一键配置（工具链 + tftp + NFS）
```

---

## 快速开始

### 1. Windows 侧（串口 + 调试）

```powershell
# 解决"禁止运行脚本"（PowerShell 执行策略默认 Restricted）
Set-ExecutionPolicy -Scope CurrentUser -ExecutionPolicy RemoteSigned

# 抓 30 秒启动日志（期间给板子断电再上电）
.\imx6ull-bootcamp\tools\serial-log.ps1 -Port COM8 -Seconds 30
```

### 2. Ubuntu 22.04 构建机（VMware，**网络必须桥接**）

```bash
cd imx6ull-bootcamp/tools
chmod +x setup-ubuntu.sh && ./setup-ubuntu.sh
```

脚本会装好工具链 + tftp + NFS，建好 `~/imx6ull/` 目录树，并打印 VM 的 IP。

### 3. 板子侧

```bash
# 登录（COM8 gadget 串口，115200 8N1）
# debian / temppwd

# 联网（插网线到 ETH1 或 ETH2，先看哪个口有载波）
cat /sys/class/net/eth1/carrier
sudo dhclient eth1 || sudo udhcpc -i eth1
ping -c 3 <Ubuntu VM 的 IP>

# 挂 NFS：改代码不用 scp
sudo mount -t nfs -o nolock,vers=3 <Ubuntu IP>:~/imx6ull/nfsroot /mnt
```

---

## 学习进度

- [x] 环境侦察：内核 / 分区 / GPIO 占用 / overlay 机制，全部实测
- [x] 路线图 M0~M7 + 调试方法论 + 23 条踩坑清单 + 自测题
- [x] 配套代码骨架（M0 / M3 / M4）+ Windows 与 Ubuntu 侧脚本
- [ ] **M-1**：VMware + Ubuntu 22.04 构建机（桥接网络）
- [ ] **M0**：交叉编译 hello → 板子上跑通
- [ ] **M1**：接 J15 USB-TTL，打断 U-Boot，读 `printenv` / `bootcmd` / `mmc list`
- [ ] **M2**：改 `uEnv.txt` 开关 overlay（零编译实验）
- [ ] **M3**：内核模块 `insmod` / `rmmod`
- [ ] **M4**：LED 字符驱动（解决 `-EBUSY`，写自己的 overlay）
- [ ] **M5**：多线程用户态程序
- [ ] **M6**：systemd + 看门狗
- [ ] **M7**：EBF-Gateway 工业数据采集网关

---

## 在另一台设备上用 DeepSeek Harness 继续

```bash
git clone https://github.com/xpfffjyt/Embedded_Linux_Learning_IMX6ULL.git
```

然后用 DeepSeek Harness 把这个目录作为 workspace 打开即可。仓库根的 [`AGENTS.md`](AGENTS.md) 里写了给 AI 的项目上下文（板子实测事实、代码约定、当前进度），主教程是 [`imx6ull-linux-roadmap.md`](imx6ull-linux-roadmap.md)。

> 建议：在另一台设备上先把 `imx6ull-bootcamp/tools/setup-ubuntu.sh` 跑一遍，环境就和这台一致了。

---

## 几点说明

- **不提交编译产物**：`.ko`、`.o`、`.dtbo`、可执行文件都由 `.gitignore` 挡住，用 `make` 自己生成。
- **文档里的 `debian/temppwd`** 是厂商镜像的**默认**账号（登录横幅自己就印着），不是你该复用的密码，改过密码的话也别提交进来。
- 本仓库只放**自己写的内容和自己板子的实测输出**，不转载野火的 PDF/视频/镜像等版权资料；需要时去[野火文档站](https://doc.embedfire.com)查。
- 引脚号、分区名、overlay 文件名都来自**本板实测**，换板子前请重新核对（尤其是 `GPIO1_IO04` 和 `imx6ull-mmc-npi.dtb`）。
