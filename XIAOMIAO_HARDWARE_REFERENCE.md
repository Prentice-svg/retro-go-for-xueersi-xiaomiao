# 学而思小猫硬件驱动参考

本文档记录当前 `RG_TARGET_XIAOMIAO` 固件的屏幕、SD 卡、按键、蜂鸣器和预留接口驱动方式，
可作为另一个 ESP32 项目的移植参考。实现位于 `components/retro-go/`，目标配置位于
`components/retro-go/targets/xiaomiao/config.h`。

## 1. 硬件总览

| 项目 | 当前配置 |
|---|---|
| MCU | ESP32-D0WD，ESP-IDF 5.3.1 |
| Flash | 4 MB |
| LCD | 2.4 英寸 ST7789，面板原始 240×320 |
| LCD 工作方向 | 横屏 320×240 |
| LCD 总线 | SPI2/HSPI，SPI mode 0，20 MHz |
| SD 卡 | SPI2 SDSPI，独立 CS |
| 音频 | GPIO14 无源蜂鸣器，LEDC PWM + ESP32 General Purpose Timer |
| 按键 | 6 个 GPIO 输入，低电平有效 |
| I2C | SDA=GPIO21、SCL=GPIO15，当前目标未启用 I2C 外设 |

## 2. ST7789 屏幕驱动

### 2.1 驱动选择

代码中没有单独命名为 `st7789.h` 的驱动，而是复用兼容的
`components/retro-go/drivers/display/ili9341.h`。选择关系如下：

```c
// components/retro-go/rg_display.c
#if RG_SCREEN_DRIVER == 0 || RG_SCREEN_DRIVER == 1
#include "drivers/display/ili9341.h"
#endif
```

小猫目标设置为：

```c
#define RG_SCREEN_DRIVER       1
#define RG_SCREEN_HOST         SPI2_HOST
#define RG_SCREEN_SPEED        SPI_MASTER_FREQ_20M
#define RG_SCREEN_WIDTH        320
#define RG_SCREEN_HEIGHT       240
#define RG_SCREEN_ROTATION     1
#define RG_SCREEN_RGB_BGR      1
```

因此它实际使用的是 **ST7789 的 SPI 命令协议**，只是复用了 Retro-Go 中原有的 ILI9341 风格
SPI/DMA 传输框架。

### 2.2 ESP32 GPIO

| 功能 | GPIO | 说明 |
|---|---:|---|
| LCD MOSI/SDA | 23 | SPI 数据输出 |
| LCD CLK/SCL | 18 | SPI 时钟 |
| LCD CS | 5 | 片选，低有效 |
| LCD DC | 4 | 命令/数据选择 |
| LCD MISO | 19 | 已配置，但当前只写屏，通常不读屏 |
| LCD RESET | 无 ESP GPIO | 复位线由排线连接；驱动另外发送软件复位 |
| LCD 背光 | 无 ESP GPIO | `RG_GPIO_LCD_BCKL` 未定义，当前不使用 LEDC 调光 |

屏幕 14P 到 10P 的接线为：

| 原 14P | 信号 | 新 10P |
|---|---|---|
| 2/5/13 | GND | 1/10 GND |
| 3 | LEDK | 8 LED- |
| 4 | LEDA | 9 LED+ |
| 6 | RESET | 3 RST |
| 7 | DC | 7 DC |
| 8 | SDA | 4 MOSI |
| 9 | SCL | 5 CLK |
| 10/11 | VCC/IOVCC | 2 3.3V |
| 12 | CS | 6 CS |
| 1/14 | NC | 不接 |

### 2.3 初始化命令

初始化函数在 `components/retro-go/drivers/display/ili9341.h:lcd_init()`：

1. 初始化 SPI2 总线和 LCD 设备，SPI mode 0，DMA 自动分配。
2. 将 DC 设置为输出并拉高。
3. 如果目标提供硬件 RESET GPIO，则执行硬件复位；本目标没有该 GPIO。
4. 发送 `0x01` 软件复位。
5. 发送 `0x3A, 0x55`，设置 RGB565/16 bpp。
6. 发送 `0x36, 0x28`，即 `BGR + MV`：
   - `0x08`：BGR 颜色顺序；
   - `0x20`：行列交换，240×320 变成横屏 320×240；
   - 当前值也修正了屏幕反向安装造成的镜像问题。
7. 执行目标的 `RG_SCREEN_INIT()`，设置 ST7789 电源、VCOM、帧率、伽马等参数。
8. 发送 `0x11` 退出睡眠，等待 10 ms，再发送 `0x29` 开启显示。

当前 `RG_SCREEN_INIT()` 使用的主要命令为：

```text
B2 0C 0C 00 33 33    Porch control
B7 35                Gate control
BB 19                VCOM setting
C0 2C                LCM control
C2 01 / C3 12 / C4 20
C6 0F                Frame rate
D0 A4 A1             Power control
E0 ...               Positive gamma
E1 ...               Negative gamma
```

### 2.4 写入像素

矩形写入流程是：

```text
0x2A + left/right 坐标     设置列窗口
0x2B + top/bottom 坐标     设置行窗口
0x2C                       Memory Write
RGB565 大端字节序像素      连续发送
```

`lcd_set_window()` 负责发送 `0x2A/0x2B/0x2C`。上层 `rg_display_write_rect()` 和
`write_update()` 将 RGB565 像素转换为屏幕需要的字节顺序后，分块送入 DMA 缓冲区。

### 2.5 SPI DMA 和刷新机制

`ili9341.h` 中的传输参数：

```c
#define SPI_TRANSACTION_COUNT 10
#define SPI_BUFFER_COUNT       5
#define LCD_BUFFER_LENGTH      (RG_SCREEN_WIDTH * 4) // 1280 pixels
#define SPI_BUFFER_LENGTH      (LCD_BUFFER_LENGTH * 2) // 2560 bytes
```

- 使用 `spi_device_queue_trans()` 异步排队；
- `spi_pre_transfer_cb()` 在每笔事务发送前设置 DC 电平：命令为 0，数据为 1；
- 小于 5 字节的命令使用 SPI transaction 内置缓冲区；
- 长数据复制到 DMA-capable 缓冲区；
- 独立 `rg_spi` 任务回收已完成的事务和 DMA 缓冲区；
- `RG_SCREEN_PARTIAL_UPDATES=1` 时，上层按行计算 checksum，只刷新变化区域；
- 缩放、居中和滤镜在 `rg_display.c` 完成，LCD 驱动本身不保存完整帧缓冲。

屏幕和 SD 卡都使用 SPI2，但 CS 不同。ESP-IDF 的 SPI 设备队列负责总线仲裁：

```text
SPI2 SCLK = GPIO18
SPI2 MOSI = GPIO23
SPI2 MISO = GPIO19
LCD CS    = GPIO5
SD  CS    = GPIO22
```

## 3. SD 卡驱动

目标使用 ESP-IDF 的 `esp_vfs_fat_sdspi_mount()`：

```c
#define RG_STORAGE_ROOT       "/sd"
#define RG_STORAGE_SDSPI_HOST SPI2_HOST
#define RG_STORAGE_SDSPI_SPEED SDMMC_FREQ_DEFAULT
#define RG_GPIO_SDSPI_MISO    GPIO_NUM_19
#define RG_GPIO_SDSPI_MOSI    GPIO_NUM_23
#define RG_GPIO_SDSPI_CLK     GPIO_NUM_18
#define RG_GPIO_SDSPI_CS      GPIO_NUM_22
```

挂载参数：

- FAT 文件系统；
- `format_if_mount_failed=false`，挂载失败不会自动格式化；
- 最多同时打开 4 个文件；
- 遇到超时、CRC 或无效响应时，会降到 `SDMMC_FREQ_PROBING` 重试；
- 根目录挂载为 `/sd`；ROM、存档、设置等都在该路径下。

推荐目录：

```text
/sd/roms/nes/
/sd/roms/gb/
/sd/roms/gbc/
/sd/roms/snes/
/sd/retro-go/config/
/sd/retro-go/saves/
```

系统启动时会先把 LCD CS 和 SD CS 拉高，避免共享 SPI 总线上未选中的设备干扰初始化。

## 4. 按键输入

小猫目标使用 GPIO 按键驱动，不使用 ADC、I2C 或串行移位寄存器：

| 按键 | GPIO | 上拉 | 有效电平 |
|---|---:|---|---:|
| UP | 2 | 内部上拉 | 0 |
| DOWN | 13 | 内部上拉 | 0 |
| LEFT | 27 | 内部上拉 | 0 |
| RIGHT | 35 | 无上拉（输入专用脚） | 0 |
| A | 34 | 无上拉（输入专用脚） | 0 |
| B | 12 | 内部上拉 | 0 |

GPIO12 是 ESP32 启动绑带脚 MTDI，GPIO34/35 是只能输入的 GPIO，移植到其他项目时需要保留
这些电气限制。

采样逻辑在 `components/retro-go/rg_input.c`：

1. `gpio_set_direction(..., GPIO_MODE_INPUT)` 设置输入；
2. 按表配置内部上拉或浮空；
3. 后台 `rg_input` 任务每 10 ms 读取一次 `gpio_get_level()`；
4. 按下和释放都需要连续 2 个采样周期确认，约 20 ms 去抖；
5. 虚拟按键使用组合键：

```text
START  = UP   + A
SELECT = DOWN + B
MENU   = LEFT + A
OPTION = RIGHT + B
```

当前虚拟键逻辑要求组合状态完全匹配 `src`，因此两个实体键同时按下时才触发。

## 5. GPIO14 蜂鸣器

目标配置：

```c
#define RG_AUDIO_USE_INT_DAC    0
#define RG_AUDIO_USE_EXT_DAC    0
#define RG_AUDIO_USE_BUZZER_PIN 14
```

这不是简单的 GPIO 翻转，而是 Retro-Go 的 PCM 蜂鸣器驱动：

1. 游戏产生单声道 `int16_t` PCM 样本；
2. 样本先放入约 50 ms 的 FreeRTOS 队列；
3. LEDC low-speed mode、channel 0、timer 0 输出到 GPIO14；
4. ESP32 General Purpose Timer group 0 / timer 0 以采样率触发中断；
5. 中断从队列取出样本，把 `[-32768,32767]` 转成 PWM duty；
6. 当前启动日志中的采样率为 32 kHz，PWM 载波也设置为 32 kHz；
7. 当前芯片得到 11 bit 左右的 PWM duty 分辨率，剩余位用于削波；
8. 静音时暂停 GPTimer、关闭中断并将 duty 设为 0。

因此，游戏中的音调来自 PCM 样本，而不是在主循环里直接调用 `gpio_set_level()`。如果另一个项目
只需要播放单音，可以复用 GPIO14 和 LEDC，但应让 PWM 载波保持在约 16 kHz 以上，再改变定时器
频率或生成方波；不要把 GPIO14 同时接到其他外设。

## 6. I2C 和电池接口现状

目标文件声明了：

```c
#define RG_GPIO_I2C_SDA GPIO_NUM_21
#define RG_GPIO_I2C_SCL GPIO_NUM_15
```

但当前 `xiaomiao/config.h` 没有定义 `RG_GAMEPAD_I2C_MAP`，也没有定义
`RG_BATTERY_DRIVER`，所以 Retro-Go 启动时不会初始化 I2C 按键或电池 ADC/I2C 驱动。
这两个 GPIO 可以在新项目中作为普通 I2C 总线重新配置，但要自行处理上拉电阻和设备地址。

## 7. 移植时最值得复用的文件

```text
components/retro-go/targets/xiaomiao/config.h  GPIO、屏幕和外设宏
components/retro-go/drivers/display/ili9341.h  SPI/DMA/LCD 窗口写入框架
components/retro-go/rg_display.c              缩放、滤镜、局部刷新
components/retro-go/rg_storage.c              SDSPI + FAT 挂载
components/retro-go/rg_input.c                 GPIO 扫描和去抖
components/retro-go/rg_audio.c                 音频 sink 选择
components/retro-go/drivers/audio/buzzer.c     GPIO14 PWM 蜂鸣器
```

如果新项目只需要点亮屏幕，最小实现就是：初始化 SPI2、配置 DC/CS、发送上述 ST7789 初始化
序列，然后按 `0x2A → 0x2B → 0x2C` 写入 RGB565 数据。需要同时接 SD 卡时，必须保留共享
SPI2 的独立 CS 和设备队列机制。
