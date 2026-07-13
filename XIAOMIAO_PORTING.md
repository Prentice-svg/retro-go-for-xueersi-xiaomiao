# 学而思小猫（XIAOMIAO）移植说明

本文说明如何把当前 Retro-Go 的小猫适配，移植到另一个 ESP32 项目或另一块相近的 ESP32 掌机上。这里的“移植”分为两种情况：

- **同一块小猫硬件，换项目**：复用 GPIO、屏幕、SD 卡、按键、音频和背光配置。
- **相近硬件，换目标板**：复制 `xiaomiao` target 后，按实际接线逐项修改。

硬件驱动的完整细节请先阅读 [XIAOMIAO_HARDWARE_REFERENCE.md](XIAOMIAO_HARDWARE_REFERENCE.md)。

## 1. 当前小猫硬件基线

| 模块 | 当前实现 |
|---|---|
| MCU | ESP32-D0WD，4 MB Flash，带 PSRAM |
| 屏幕 | 2.4 英寸 ST7789，原始 240×320，固件横屏 320×240 |
| LCD SPI | SPI2，MOSI=GPIO23，CLK=GPIO18，CS=GPIO5，DC=GPIO4 |
| SD SPI | 与 LCD 共用 SPI2，MISO=19、MOSI=23、CLK=18、CS=22 |
| 音频 | MAX98357A，I2S0：BCLK=25、LRC/WS=32、DIN=33 |
| 背光 | GPIO14，LEDC PWM，当前为非反相极性 |
| 按键 | UP=2、DOWN=13、LEFT=27、RIGHT=35、A=34、B=12 |
| 虚拟键 | MENU=UP+DOWN+B，START=UP+DOWN，SELECT=LEFT+RIGHT |
| 默认语言 | 简体中文，FusionPixel12 字体 |

GPIO34、GPIO35 只能输入，GPIO12 是启动绑带脚。移植时不能只看软件定义，还要确认上电状态不会影响启动。

## 2. 从零创建一个新 target

以 `myboard` 为例：

```powershell
Copy-Item components/retro-go/targets/xiaomiao components/retro-go/targets/myboard -Recurse
```

实际操作时请直接复制到新的目录，不要覆盖原来的 `xiaomiao`。至少需要修改：

```text
components/retro-go/targets/myboard/config.h
components/retro-go/targets/myboard/env.py
components/retro-go/targets/myboard/sdkconfig
```

### 2.1 `config.h` 必改项目

```c
#define RG_TARGET_NAME       "MYBOARD"
#define RG_SCREEN_DRIVER     1
#define RG_SCREEN_HOST       SPI2_HOST
#define RG_SCREEN_SPEED      SPI_MASTER_FREQ_20M
#define RG_SCREEN_WIDTH      320
#define RG_SCREEN_HEIGHT     240
```

然后根据实际接线修改：

1. `RG_GPIO_LCD_MOSI/CLK/CS/DC/BCKL`：屏幕数据、时钟、片选、命令脚和背光脚。
2. `RG_SCREEN_ROTATION`、`RG_SCREEN_RGB_BGR`：解决上下翻转、左右镜像和 RGB/BGR 色序。
3. `RG_SCREEN_INIT()`：ST7789 的电源、帧率、伽马和 VCOM 参数。
4. `RG_GPIO_SDSPI_*`：SD 卡是否与屏幕共用 SPI 总线，CS 必须独立。
5. `RG_GAMEPAD_GPIO_MAP`：实体按键 GPIO、上拉方式和有效电平。
6. `RG_GAMEPAD_VIRT_MAP`：MENU、START、SELECT 等组合键。
7. `RG_GPIO_SND_I2S_BCK/WS/DATA`：外部 I2S 功放的三根信号线。
8. `RG_GPIO_LCD_BCKL` 和 PWM 极性：确认亮度值越大是否越亮。

### 2.2 ST7789 初始化和刷新同步

小猫使用兼容 ILI9341 框架的 `components/retro-go/drivers/display/ili9341.h`。不要只把分辨率改成 320×240，还必须确认：

- `MADCTL (0x36)` 的 MV、MX、MY、BGR 位与面板安装方向匹配。
- `SWRESET (0x01)` 后等待至少 120 ms。
- `SLPOUT (0x11)` 后等待至少 120 ms。
- 异步 SPI 队列在初始化和换窗口前完成 `spi_flush()`。
- `lcd_sync()` 不能再是空函数，否则可能出现半屏、残影或偶发黑屏。

如果新屏幕上下反了，优先尝试 `RG_SCREEN_ROTATION`；如果文字左右镜像，再检查 `MADCTL` 中的 MX/MY，而不是修改字体或坐标。

### 2.3 音频与背光

MAX98357A 不需要 MCLK，只需要：

```text
ESP32 GPIO25  -> MAX98357A BCLK
ESP32 GPIO32  -> MAX98357A LRC/WS
ESP32 GPIO33  -> MAX98357A DIN
GND           -> GND
```

`RG_AUDIO_USE_INT_DAC` 应设为 `0`，`RG_AUDIO_USE_EXT_DAC` 设为 `1`。GPIO14 已经改作背光后，必须保持：

```c
#define RG_AUDIO_USE_BUZZER_PIN 0
#define RG_SCREEN_BACKLIGHT     1
#define RG_GPIO_LCD_BCKL        GPIO_NUM_14
```

若亮度反向，在 `ili9341.h` 的 LEDC duty 输出处调整极性；不要同时把 GPIO14 配成蜂鸣器和背光。

## 3. 中文菜单和字体移植

中文显示不是只改语言编号，还需要三部分同时存在：

1. `translations.h` 中有对应简体中文条目。
2. `rg_gui_set_language_id()` 默认选择 `RG_LANG_CN`。
3. 字体包含所需汉字，推荐 `FusionPixel12`，必要时使用 `ZenHei16` 回退。

新增界面文字时：

```c
rg_gui_draw_text(..., _("设置"), ...);
```

并在翻译表中增加对应条目。不要把中文字符串强制按 8 像素 ASCII 宽度截断；中文通常是全角字宽，菜单宽度和换行应按字体实际测量结果计算。

如果屏幕出现点阵、缺字或一行只显示半截，依次检查：

- 是否刷入了包含中文字体的完整 launcher 镜像。
- `CONFIG_FATFS_API_ENCODING_UTF_8=y` 是否启用。
- 字体 C 文件是否重新生成并参与 launcher 编译。
- 字体高度、字宽和菜单可用宽度是否匹配。

## 4. 编译、刷写和验证

在 ESP-IDF Python 环境中执行：

```powershell
# 构建小猫完整镜像
python rg_tool.py --target xiaomiao build-img

# 构建并刷写到指定串口
python rg_tool.py --target xiaomiao --port COM8 install

# 只刷 launcher（调试菜单和字体时更快）
python rg_tool.py --target xiaomiao --port COM8 flash launcher
```

也可以手动刷写完整镜像：

```powershell
esptool.py --chip esp32 --port COM8 --baud 921600 `
  write_flash --flash_size detect 0x0 retro-go_*_xiaomiao.img
```

刷写后应通过串口确认：

```text
Display ready.
Audio ready.
Retro-Go ready.
```

如果出现 `PNG decoding failed: 58`，先检查默认主题图片是否被重新压缩、是否运行过 `python tools/gen_images.py`，再检查 SD 卡中的自定义主题文件。

## 5. 移植到另一个独立 ESP32 项目

如果不使用 Retro-Go，只想复用硬件驱动，可以按下面的最小顺序移植：

1. 初始化 SPI2，配置 MOSI、CLK、CS、DC。
2. 拉高 CS，设置 DC 为输出。
3. 发送 ST7789 `SWRESET`，等待 120 ms。
4. 发送 `COLMOD=0x55`，设置 RGB565。
5. 发送适合面板的 `MADCTL` 和电源/伽马参数。
6. 发送 `SLPOUT`，等待 120 ms，再发送 `DISPON`。
7. 写像素时依次发送 `CASET (0x2A)`、`RASET (0x2B)`、`RAMWR (0x2C)`。
8. RGB565 数据按屏幕要求发送高字节在前。
9. 如果使用 DMA，等待上一帧传输完成后再复用 DMA 缓冲区。
10. 用 LEDC 输出背光，用 I2S 输出 MAX98357A 音频。

最小的屏幕窗口写入顺序：

```text
CS=0
DC=0: 2A + x_start + x_end
DC=0: 2B + y_start + y_end
DC=0: 2C
DC=1: RGB565 pixel data
CS=1
```

## 6. 移植检查清单

- [ ] MCU、Flash、PSRAM 容量确认
- [ ] 屏幕控制器和原始分辨率确认
- [ ] SPI 屏幕与 SD 卡的 CS 分离
- [ ] ST7789 旋转、镜像、BGR 色序确认
- [ ] SWRESET/SLPOUT 延时和 SPI 队列同步确认
- [ ] 按键有效电平和启动绑带脚确认
- [ ] MAX98357A 三根 I2S 信号线确认
- [ ] 背光 PWM 极性确认
- [ ] 中文字体和 UTF-8 文件名确认
- [ ] 完整镜像刷写并保存串口启动日志
- [ ] 连续冷启动、切换分类、进入/退出游戏测试

## 7. 相关文件

| 文件 | 用途 |
|---|---|
| `components/retro-go/targets/xiaomiao/config.h` | 小猫 GPIO、屏幕、音频、按键配置 |
| `components/retro-go/drivers/display/ili9341.h` | ST7789 SPI/DMA、窗口写入和背光 |
| `components/retro-go/drivers/audio/i2s.c` | MAX98357A I2S 音频输出 |
| `components/retro-go/rg_input.c` | GPIO 按键扫描和组合键 |
| `components/retro-go/rg_storage.c` | SDSPI + FAT 挂载 |
| `launcher/main/gui.c` | 菜单、分类页、图片和中文界面 |
| `themes/default/` | 默认背景、横幅、分类图标 |
| `tools/gen_images.py` | 将 PNG 重新生成 `launcher/main/images.c` |
| `XIAOMIAO_HARDWARE_REFERENCE.md` | 当前硬件驱动的详细参考 |
| `PORTING.md` | Retro-Go 通用英文移植说明 |
