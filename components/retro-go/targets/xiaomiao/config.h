#define RG_TARGET_NAME             "XIAOMIAO"
#define RG_DEFAULT_TIMEZONE        "UTC+8"
#define RG_FONT_DEFAULT            RG_FONT_FUSIONPIXEL_12
#define RG_LANG_DEFAULT            RG_LANG_CN
// Bump this when changing the target's initial language so an existing
// English global.json is migrated to Simplified Chinese once.
#define RG_LANG_CONFIG_VERSION     1
// Likewise migrate an old Basic 8 setting to the CJK-capable Fusion Pixel font.
#define RG_FONT_CONFIG_VERSION     1

#define RG_STORAGE_ROOT            "/sd"
#define RG_STORAGE_SDSPI_HOST      SPI2_HOST
#define RG_STORAGE_SDSPI_SPEED     SDMMC_FREQ_DEFAULT

#define RG_AUDIO_USE_INT_DAC       0
#define RG_AUDIO_USE_EXT_DAC       0
// This must be a preprocessor integer (not GPIO_NUM_14, which is an enum),
// otherwise the buzzer backend is compiled out by its #if guard.
#define RG_AUDIO_USE_BUZZER_PIN    14

// 2.4-inch ST7789, 240x320 panel used in landscape (320x240) on SPI2.
// The 10-pin adapter preserves the original Xiaomiao SPI wiring:
// MOSI=GPIO23, CLK=GPIO18, CS=GPIO5, DC=GPIO4.  The panel reset is tied
// through the original 14-pin display connector and the controller also
// receives the mandatory software reset during initialization.
#define RG_SCREEN_DRIVER           1
#define RG_SCREEN_HOST             SPI2_HOST
#define RG_SCREEN_SPEED            SPI_MASTER_FREQ_20M
#define RG_SCREEN_BACKLIGHT        0
#define RG_SCREEN_WIDTH            320
#define RG_SCREEN_HEIGHT           240
#define RG_SCREEN_ROTATION         1
#define RG_SCREEN_RGB_BGR          1
#define RG_SCREEN_VISIBLE_AREA     {0, 0, 0, 0}
#define RG_SCREEN_SAFE_AREA        {0, 0, 0, 0}

// ST7789 240x320 initialization. RG_SCREEN_ROTATION=1 emits MADCTL 0x28:
// landscape, BGR, with the horizontal mirror corrected for this panel.
#define RG_SCREEN_INIT()                                                                                         \
    ILI9341_CMD(0xB2, 0x0C, 0x0C, 0x00, 0x33, 0x33); /* Porch control */                                      \
    ILI9341_CMD(0xB7, 0x35);                         /* Gate control */                                       \
    ILI9341_CMD(0xBB, 0x19);                         /* VCOM setting */                                       \
    ILI9341_CMD(0xC0, 0x2C);                         /* LCM control */                                        \
    ILI9341_CMD(0xC2, 0x01);                         /* VDV/VRH enable */                                     \
    ILI9341_CMD(0xC3, 0x12);                         /* VRH setting */                                        \
    ILI9341_CMD(0xC4, 0x20);                         /* VDV setting */                                        \
    ILI9341_CMD(0xC6, 0x0F);                         /* Frame rate */                                         \
    ILI9341_CMD(0xD0, 0xA4, 0xA1);                   /* Power control */                                      \
    ILI9341_CMD(0xE0, 0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F, 0x54, 0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23);    \
    ILI9341_CMD(0xE1, 0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F, 0x44, 0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23)

// Input: GPIO34/35 are input-only (no pullup), GPIO12 is boot-strap (MTDI)
#define RG_GAMEPAD_GPIO_MAP {\
    {RG_KEY_UP,     .num = GPIO_NUM_2,  .pullup = 1, .level = 0},\
    {RG_KEY_DOWN,   .num = GPIO_NUM_13, .pullup = 1, .level = 0},\
    {RG_KEY_LEFT,   .num = GPIO_NUM_27, .pullup = 1, .level = 0},\
    {RG_KEY_RIGHT,  .num = GPIO_NUM_35, .pullup = 0, .level = 0},\
    {RG_KEY_A,      .num = GPIO_NUM_34, .pullup = 0, .level = 0},\
    {RG_KEY_B,      .num = GPIO_NUM_12, .pullup = 1, .level = 0},\
}

// Virtual combos for Start/Select/Menu.
// Keep the three-key MENU entry before START because the matching logic
// replaces an exact source state as soon as it finds a match.
#define RG_GAMEPAD_VIRT_MAP {\
    {RG_KEY_MENU,   .src = RG_KEY_UP    | RG_KEY_DOWN | RG_KEY_B},\
    {RG_KEY_START,  .src = RG_KEY_UP    | RG_KEY_DOWN},\
    {RG_KEY_SELECT, .src = RG_KEY_LEFT  | RG_KEY_RIGHT},\
}

// GPIO pin assignments
#define RG_GPIO_I2C_SDA             GPIO_NUM_21
#define RG_GPIO_I2C_SCL             GPIO_NUM_15

#define RG_GPIO_LCD_MISO            GPIO_NUM_19
#define RG_GPIO_LCD_MOSI            GPIO_NUM_23
#define RG_GPIO_LCD_CLK             GPIO_NUM_18
#define RG_GPIO_LCD_CS              GPIO_NUM_5
#define RG_GPIO_LCD_DC              GPIO_NUM_4
// No backlight or ESP GPIO reset; the display driver issues a software reset.

#define RG_GPIO_SDSPI_MISO          GPIO_NUM_19
#define RG_GPIO_SDSPI_MOSI          GPIO_NUM_23
#define RG_GPIO_SDSPI_CLK           GPIO_NUM_18
#define RG_GPIO_SDSPI_CS            GPIO_NUM_22

#define RG_UPDATER_ENABLE           1
#define RG_UPDATER_APPLICATION      RG_APP_FACTORY
#define RG_UPDATER_DOWNLOAD_LOCATION RG_STORAGE_ROOT "/xiaomiao/firmware"
