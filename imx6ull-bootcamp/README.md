# imx6ull-bootcamp

配套代码 / 工具，对应上级目录的 [`imx6ull-linux-roadmap.md`](../imx6ull-linux-roadmap.md)。

**板子实测状态**（2026-11-11 从串口读出，非猜测）：

| 项目 | 值 |
|---|---|
| 内核 | `4.19.35-imx6` |
| 系统 | Debian 10 buster，`embedfire.com Debian Image 2025-08-23` |
| 登录 | `debian` / **`temppwd`**（登录横幅里直接写着），`sudo` 需要密码 |
| 串口 | `COM8` = **USB gadget 串口（`ttyGS0`）**，Linux 起来后才有；真正的 UART `ttymxc0`（U-Boot 走这里）尚未接到 PC，见路线图 §7.0 |
| 模块目录 | `/lib/modules/4.19.35-imx6/`，**无 `build` 软链** → 只能在主机交叉编译 |
| LED | RGB 已被 `leds-gpio` 占用：红 `GPIO1_IO04`、绿 `GPIO3_IO20`、蓝 `GPIO3_IO19` |

---

## 目录

| 路径 | 用途 | 里程碑 |
|---|---|---|
| `tools/serial-log.ps1` | Windows 串口探针 / 启动日志抓取 | 前置 |
| `tools/setup-ubuntu.sh` | Ubuntu 22.04 构建机一键配置（工具链 + tftp + NFS + 目录） | 前置 |
| `m0-hello/` | 第一个交叉编译程序 | M0 |
| `m3-hello-module/` | 第一个内核模块（含模块参数） | M3 |
| `m4-led-driver/` | LED 字符驱动 + **overlay** + 用户态测试 | M4 |

---

## 步骤 0：先解决 PowerShell 执行策略

直接运行会报「**在此系统上禁止运行脚本**」（`UnauthorizedAccess`）。这不是权限问题，是执行策略默认 `Restricted`。

```powershell
# 方式 A：只影响这一条命令（零改动，推荐先用）
powershell -ExecutionPolicy Bypass -File .\tools\serial-log.ps1 -Port COM8 -Seconds 30

# 方式 B：永久解决（仅当前用户，不需要管理员）
Set-ExecutionPolicy -Scope CurrentUser -ExecutionPolicy RemoteSigned
```

> 别用 `Unblock-File`（那是清"来自网络"标记的，对本问题无效），也别用"以管理员身份运行"（策略问题不是权限问题）。

## 步骤 1：串口

```powershell
# 板上电过程中抓 30 秒，得到完整启动日志
.\tools\serial-log.ps1 -Port COM8 -Seconds 30

# 板子已启动，发回车看提示符
.\tools\serial-log.ps1 -Port COM8 -Seconds 8 -Nudge
```

若报「访问被拒绝」：有程序独占 COM8（串口助手 / MobaXterm / Xshell / VS Code Serial Monitor），关掉即可。

## 步骤 2：M0 —— 打通第一次闭环

Ubuntu 构建机上：

```bash
cd m0-hello
make                  # 默认 arm-linux-gnueabihf-
make check            # file + readelf，确认是 ARM、依赖齐全
```

> ⚠️ 在 x86 的 Ubuntu 上**不要用 `ldd`** 检查 ARM 程序，它跑不了。用 `make check`。

部署：`make deploy TARGET_IP=192.168.1.50`，或直接 `cp hello ~/imx6ull/nfsroot/` 走 NFS。

板子上：

```bash
./hello
# release : 4.19.35-imx6
# machine : armv7l
```

报 `No such file or directory` 但文件在 → 动态链接器缺失，用 `make static` 先验证链路。

## 步骤 3：M3 —— 第一个内核模块

```bash
cd m3-hello-module
make KDIR=$HOME/imx6ull/kernel/linux-imx
```

> 前置条件：`$KDIR` 的内核树**必须已完整编译过一次**（有 `Module.symvers`、`include/generated/autoconf.h`）。
> 关键：内核版本必须和你板子上跑的 `4.19.35-imx6` 一致，否则 `insmod` 报 `Invalid module format`。
> 版本不对时，可以用板子上的 `/boot/kernel/config-4.19.35-imx6` 作为起点（这是出厂内核的真实配置）。

板子上：

```bash
sudo insmod hello_module.ko count=3 who="EBF"
dmesg | tail -5
cat /sys/module/hello_module/parameters/count
sudo rmmod hello_module
```

## 步骤 4：M4 —— LED 驱动（**注意这里和常见教程不一样**）

你的板子用 **U-Boot + uEnv.txt + dtbo overlay** 组织设备树，不是重编整份 dts。顺序不能错：

### 4.1 先释放 GPIO（否则 probe 报 `-EBUSY`）

红 LED 的 `GPIO1_IO04` 已被 `leds-gpio` 占用，来自 `imx-fire-led.dtbo`：

```bash
# 板子上
sudo cp /boot/uEnv.txt /boot/uEnv.txt.bak
sudo vim /boot/uEnv.txt
# 注释掉这行：
# dtoverlay=/usr/lib/linux-image-4.19.35-imx6/overlays/imx-fire-led.dtbo
sudo reboot

gpioinfo | grep -E 'red|green|blue'   # 应搜不到 → 引脚已释放
```

### 4.2 编译并安装 overlay

```bash
# Ubuntu 上
cd m4-led-driver
make KDIR=$HOME/imx6ull/kernel/linux-imx dtbo   # 生成 ebf-led.dtbo
scp ebf-led.dtbo root@192.168.1.50:/tmp/

# 板子上
sudo cp /tmp/ebf-led.dtbo /usr/lib/linux-image-4.19.35-imx6/overlays/
echo 'dtoverlay=/usr/lib/linux-image-4.19.35-imx6/overlays/ebf-led.dtbo' | sudo tee -a /boot/uEnv.txt
sudo reboot

ls /proc/device-tree | grep ebf       # 节点出现 = overlay 生效
```

### 4.3 编译并加载驱动

```bash
# Ubuntu 上
make KDIR=$HOME/imx6ull/kernel/linux-imx ko
make user

# 板子上
sudo insmod led_drv.ko
ls -l /dev/ebf_led                     # 存在 = compatible 匹配成功
sudo sh -c 'echo 1 > /dev/ebf_led'     # 红灯亮
sudo sh -c 'echo 0 > /dev/ebf_led'     # 红灯灭
sudo ./led_test                        # 闪 5 次
dmesg | grep ebf
```

**灯不亮怎么查**（按顺序）：

1. `dmesg | grep ebf` 无输出 → `compatible` 不匹配，或 dtbo 没装进 uEnv.txt
2. `ls /proc/device-tree | grep ebf` 为空 → overlay 根本没加载成功（`-@` 漏了？dtbo 路径写错？uEnv.txt 备份还原了？）
3. 节点在、`ls /dev/ebf_led` 没有 → `cdev`/`device_create` 失败，看 `dmesg` 报错
4. `dmesg` 里 `failed to get gpios: -16` → `-EBUSY`，4.1 那步没做或没生效
5. 有节点但灯不动 → 引脚号错（对着 `gpioinfo` 核对）或极性反了（去掉 `GPIO_ACTIVE_LOW` 试）

## 关于代码风格

- 源码注释用英文（ASCII）：老工具链和串口控制台对 UTF-8 不友好。
- 文档用中文。
- 驱动是**教学代码**，故意保留了简化（单实例、无并发保护）。M4 加分题就是补 `mutex`、`poll`、`sysfs`、`input`。
