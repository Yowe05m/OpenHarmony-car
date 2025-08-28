#include "ssd1306_oled.h"
#include "iot_gpio.h"       // HAL GPIO
#include "hi_io.h"          // IO 复用
#include "iot_i2c.h"        // HAL I2C
#include "ssd1306.h"        // SSD1306 驱动接口
#include "ssd1306_fonts.h"  // 字体

// 根据你板子的接线改这里
#define OLED_SDA_PIN        HI_IO_NAME_GPIO_13
#define OLED_SCL_PIN        HI_IO_NAME_GPIO_14
#define OLED_I2C_BUS        0
#define OLED_I2C_BAUDRATE   400000  // demo 里通常是 400kHz

void Oled_Init(void)
{
    // 1) 配置 SDA/SCL 引脚为 I2C0
    IoTGpioInit(OLED_SDA_PIN);
    IoTGpioInit(OLED_SCL_PIN);
    hi_io_set_func(OLED_SDA_PIN, HI_IO_FUNC_GPIO_13_I2C0_SDA);
    hi_io_set_func(OLED_SCL_PIN, HI_IO_FUNC_GPIO_14_I2C0_SCL);

    // 2) 初始化 I2C 总线
    IoTI2cInit(OLED_I2C_BUS, OLED_I2C_BAUDRATE);

    // 3) 初始化 OLED，清屏
    ssd1306_Init();
    ssd1306_Fill(Black);
    ssd1306_UpdateScreen();
}

void Oled_ShowString(uint8_t x, uint8_t y, const char *str)
{
    // 在 (x,y) 写入，然后刷新
    // ssd1306_Fill(Black);               // 如果想保留前面内容，可以去掉这行
    ssd1306_SetCursor(x, y);
    ssd1306_DrawString(str, Font_7x10, White);
    ssd1306_UpdateScreen();
}
