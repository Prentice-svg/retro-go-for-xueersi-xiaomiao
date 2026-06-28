#define RG_TARGET_NAME             "XIAOMIAO"
#define RG_DEFAULT_TIMEZONE        "UTC+8"
#define RG_FONT_DEFAULT            RG_FONT_BASIC_8

#define RG_STORAGE_ROOT            "/sd"
#define RG_STORAGE_SDSPI_HOST      SPI2_HOST
#define RG_STORAGE_SDSPI_SPEED     SDMMC_FREQ_DEFAULT

#define RG_AUDIO_USE_INT_DAC       0
#define RG_AUDIO_USE_EXT_DAC       0
#define RG_AUDIO_USE_BUZZER_PIN    GPIO_NUM_14

// ST7735 160x128 landscape on SPI2 (HSPI conflicts with PSRAM)
#define RG_SCREEN_DRIVER           2
#define RG_SCREEN_HOST             SPI2_HOST
#define RG_SCREEN_SPEED            SPI_MASTER_FREQ_20M
#define RG_SCREEN_BACKLIGHT        0
#define RG_SCREEN_WIDTH            160
#define RG_SCREEN_HEIGHT           128
#define RG_SCREEN_MADCTL           0x60
#define RG_SCREEN_COL_OFFSET       0
#define RG_SCREEN_ROW_OFFSET       0
#define RG_SCREEN_VISIBLE_AREA     {0, 0, 0, 0}
#define RG_SCREEN_SAFE_AREA        {0, 0, 0, 0}

// Input: GPIO34/35 are input-only (no pullup), GPIO12 is boot-strap (MTDI)
#define RG_GAMEPAD_GPIO_MAP {\
    {RG_KEY_UP,     .num = GPIO_NUM_2,  .pullup = 1, .level = 0},\
    {RG_KEY_DOWN,   .num = GPIO_NUM_13, .pullup = 1, .level = 0},\
    {RG_KEY_LEFT,   .num = GPIO_NUM_27, .pullup = 1, .level = 0},\
    {RG_KEY_RIGHT,  .num = GPIO_NUM_35, .pullup = 0, .level = 0},\
    {RG_KEY_A,      .num = GPIO_NUM_34, .pullup = 0, .level = 0},\
    {RG_KEY_B,      .num = GPIO_NUM_12, .pullup = 1, .level = 0},\
}

// Virtual combos for Start/Select/Menu/Option
#define RG_GAMEPAD_VIRT_MAP {\
    {RG_KEY_START,  .src = RG_KEY_UP    | RG_KEY_A},\
    {RG_KEY_SELECT, .src = RG_KEY_DOWN  | RG_KEY_B},\
    {RG_KEY_MENU,   .src = RG_KEY_LEFT  | RG_KEY_A},\
    {RG_KEY_OPTION, .src = RG_KEY_RIGHT | RG_KEY_B},\
}

// GPIO pin assignments
#define RG_GPIO_I2C_SDA             GPIO_NUM_21
#define RG_GPIO_I2C_SCL             GPIO_NUM_15

#define RG_GPIO_LCD_MISO            GPIO_NUM_19
#define RG_GPIO_LCD_MOSI            GPIO_NUM_23
#define RG_GPIO_LCD_CLK             GPIO_NUM_18
#define RG_GPIO_LCD_CS              GPIO_NUM_5
#define RG_GPIO_LCD_DC              GPIO_NUM_4
// No backlight or HW reset; st7735 driver uses software reset instead

#define RG_GPIO_SDSPI_MISO          GPIO_NUM_19
#define RG_GPIO_SDSPI_MOSI          GPIO_NUM_23
#define RG_GPIO_SDSPI_CLK           GPIO_NUM_18
#define RG_GPIO_SDSPI_CS            GPIO_NUM_22

#define RG_UPDATER_ENABLE           1
#define RG_UPDATER_APPLICATION      RG_APP_FACTORY
#define RG_UPDATER_DOWNLOAD_LOCATION RG_STORAGE_ROOT "/xiaomiao/firmware"
