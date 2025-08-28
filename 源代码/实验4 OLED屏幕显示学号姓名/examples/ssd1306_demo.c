/*
 * Copyright (c) 2020, HiHope Community.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <unistd.h>

#include "ohos_init.h"
#include "cmsis_os2.h"
#include "iot_gpio.h"
#include "iot_pwm.h"
#include "iot_i2c.h"
#include "iot_errno.h"

#include "ssd1306.h"
#include "ssd1306_tests.h"
#include "ssd1306_fonts.h"

#include "hi_io.h"

#define OLED_I2C_BAUDRATE 400*1000

void TestGetTick(void)
{
    for (int i = 0; i < 20; i++) {
        usleep(10*1000);
        printf("HAL_GetTick(): %d\r\n", HAL_GetTick());
    }

    for (int i = 0; i < 20; i++) {
        HAL_Delay(25);
        printf(" HAL_GetTick(): %d\r\n", HAL_GetTick());
    }
}


/**
 * 汉字字模在线： https://www.23bei.com/tool-223.html
 * 数据排列：从左到右从上到下
 * 取模方式：横向8位左高位
**/
void TestDrawChinese1(void)
{
    const uint32_t W = 16, H = 16;
    uint8_t fonts[][32] = {
        {
            /*-- ID:0,字符:"你",ASCII编码:C4E3,对应字:宽x高=16x16,画布:宽W=16 高H=16,共32字节*/
            0x11,0x00,0x11,0x00,0x11,0x00,0x23,0xFC,0x22,0x04,0x64,0x08,0xA8,0x40,0x20,0x40,
            0x21,0x50,0x21,0x48,0x22,0x4C,0x24,0x44,0x20,0x40,0x20,0x40,0x21,0x40,0x20,0x80,
        },{
            /*-- ID:1,字符:"好",ASCII编码:BAC3,对应字:宽x高=16x16,画布:宽W=16 高H=16,共32字节*/
            0x10,0x00,0x11,0xFC,0x10,0x04,0x10,0x08,0xFC,0x10,0x24,0x20,0x24,0x24,0x27,0xFE,
            0x24,0x20,0x44,0x20,0x28,0x20,0x10,0x20,0x28,0x20,0x44,0x20,0x84,0xA0,0x00,0x40,
        },{
            /*-- ID:2,字符:"鸿",ASCII编码:BAE8,对应字:宽x高=16x16,画布:宽W=16 高H=16,共32字节*/
            0x40,0x20,0x30,0x48,0x10,0xFC,0x02,0x88,0x9F,0xA8,0x64,0x88,0x24,0xA8,0x04,0x90,
            0x14,0x84,0x14,0xFE,0xE7,0x04,0x3C,0x24,0x29,0xF4,0x20,0x04,0x20,0x14,0x20,0x08,
        },{
            /*-- ID:3,字符:"蒙",ASCII编码:C3C9,对应字:宽x高=16x16,画布:宽W=16 高H=16,共32字节*/
            0x04,0x48,0x7F,0xFC,0x04,0x40,0x7F,0xFE,0x40,0x02,0x8F,0xE4,0x00,0x00,0x7F,0xFC,
            0x06,0x10,0x3B,0x30,0x05,0xC0,0x1A,0xA0,0x64,0x90,0x18,0x8E,0x62,0x84,0x01,0x00
        }
    };

    ssd1306_Fill(Black);
    for (size_t i = 0; i < sizeof(fonts)/sizeof(fonts[0]); i++) {
        ssd1306_DrawRegion(i * W, 0, W, H, fonts[i], sizeof(fonts[0]), W);
    }
    ssd1306_UpdateScreen();
    sleep(1);
}

void TestDrawChinese2(void)
{
    const uint32_t W = 12, H = 12, S = 16;
    uint8_t fonts[][24] = {
        {
            /*-- ID:0,字符:"你",ASCII编码:C4E3,对应字:宽x高=12x12,画布:宽W=16 高H=12,共24字节*/
            0x12,0x00,0x12,0x00,0x27,0xF0,0x24,0x20,0x69,0x40,0xA1,0x00,0x25,0x40,0x25,0x20,
            0x29,0x10,0x31,0x10,0x25,0x00,0x22,0x00,
        },{
            /*-- ID:1,字符:"好",ASCII编码:BAC3,对应字:宽x高=12x12,画布:宽W=16 高H=12,共24字节*/
            0x20,0x00,0x27,0xE0,0x20,0x40,0xF8,0x80,0x48,0x80,0x48,0xA0,0x57,0xF0,0x50,0x80,
            0x30,0x80,0x28,0x80,0x4A,0x80,0x81,0x00,
        },{
            /*-- ID:2,字符:"鸿",ASCII编码:BAE8,对应字:宽x高=12x12,画布:宽W=16 高H=12,共24字节*/
            0x00,0x40,0x80,0x80,0x5D,0xE0,0x09,0x20,0xC9,0xA0,0x09,0x60,0x29,0x00,0xCD,0xF0,
            0x58,0x10,0x43,0xD0,0x40,0x10,0x40,0x60,
        },{
            /*-- ID:3,字符:"蒙",ASCII编码:C3C9,对应字:宽x高=12x12,画布:宽W=16 高H=12,共24字节*/
            0x09,0x00,0x7F,0xE0,0x09,0x00,0x7F,0xF0,0x80,0x10,0x7F,0xE0,0x0C,0x40,0x32,0x80,
            0xC7,0x00,0x0A,0x80,0x32,0x70,0xC6,0x20
        }
    };

    ssd1306_Fill(Black);
    for (size_t i = 0; i < sizeof(fonts)/sizeof(fonts[0]); i++) {
        ssd1306_DrawRegion(i * H, 0, W, H, fonts[i], sizeof(fonts[0]), S);
    }
    ssd1306_UpdateScreen();
    sleep(1);
}

void TestDrawChinese3(void)
{
    // 1) 定义你的字宽/字高（以像素为单位）
    const uint32_t W = 16/* 字宽，比如 16 */;
    const uint32_t H = 16/* 字高，比如 16 */;
    // 2) 计算每个字符占用的字节数 = W * H / 8
    //    然后把你的字模数据填到下面的二维数组里
    //    举例：如果是 16×16，则是 32 字节；如果 12×12，则 18 字节，以此类推
    uint8_t myFonts[][32] = {
        {
            /*-- ID:0,字符:"马",ASCII编码:C2ED,对应字:宽x高=16x16,画布:宽W=16 高H=16,共32字节*/
            0x00,0x20,0x3F,0xF0,0x00,0x20,0x08,0x20,0x08,0x20,0x08,0x20,0x08,0x20,0x08,0x24,
            0x0F,0xFE,0x00,0x04,0x00,0x24,0xFF,0xF4,0x00,0x04,0x00,0x04,0x00,0x28,0x00,0x10
        },
        {
            /*-- ID:1,字符:"成",ASCII编码:B3C9,对应字:宽x高=16x16,画布:宽W=16 高H=16,共32字节*/
            0x00,0x80,0x00,0xA0,0x00,0x90,0x3F,0xFC,0x20,0x80,0x20,0x80,0x20,0x84,0x3E,0x44,
            0x22,0x48,0x22,0x48,0x22,0x30,0x2A,0x20,0x24,0x62,0x40,0x92,0x81,0x0A,0x00,0x06
        },
        {
            /*-- ID:2,字符:"龙",ASCII编码:C1FA,对应字:宽x高=16x16,画布:宽W=16 高H=16,共32字节*/
            0x02,0x00,0x02,0x40,0x02,0x20,0x02,0x04,0xFF,0xFE,0x02,0x80,0x02,0x88,0x04,0x88,
            0x04,0x90,0x04,0xA0,0x08,0xC0,0x08,0x82,0x11,0x82,0x16,0x82,0x20,0x7E,0x40,0x00
        },

    };

    // 3) 清屏（先把缓冲区全填黑）
    ssd1306_Fill(Black);

    // 4) 循环绘制每个字模到缓冲区
    //    x 偏移 = i * W；y 固定为 0
    //    stride 参数传 W，即每行 W 位（内部会自动按 8 位分组）
    for (size_t i = 0; i < sizeof(myFonts)/sizeof(myFonts[0]); i++) {
        ssd1306_DrawRegion(
            /* x */ i * W,
            /* y */ 0,
            /* w */ W,
            /* h */ H,
            /* data */ myFonts[i],
            /* size */ sizeof(myFonts[0]),
            /* stride */ W
        );
    }
    
    // 到第二行
    ssd1306_SetCursor(0, 20);
    ssd1306_DrawString("12345678901234", Font_7x10, White);

    // 5) 一次性把缓冲区刷新到屏幕
    ssd1306_UpdateScreen();
}


void Ssd1306TestTask(void* arg)
{
    (void) arg;
    IoTGpioInit(HI_IO_NAME_GPIO_13);
    IoTGpioInit(HI_IO_NAME_GPIO_14);

    hi_io_set_func(HI_IO_NAME_GPIO_13, HI_IO_FUNC_GPIO_13_I2C0_SDA);
    hi_io_set_func(HI_IO_NAME_GPIO_14, HI_IO_FUNC_GPIO_14_I2C0_SCL);
    
    IoTI2cInit(0, OLED_I2C_BAUDRATE);

    //WatchDogDisable();

    usleep(20*1000);
    ssd1306_Init();
    ssd1306_Fill(Black);
    ssd1306_SetCursor(0, 0);
    // ssd1306_DrawString("Hello HarmonyOS!", Font_7x10, White);

    uint32_t start = HAL_GetTick();
    ssd1306_UpdateScreen();
    uint32_t end = HAL_GetTick();
    printf("ssd1306_UpdateScreen time cost: %d ms.\r\n", end - start);

    // TestDrawChinese1();
    // TestDrawChinese2();

    TestDrawChinese3();

    TestGetTick();
    
    // while (1) {
    //     ssd1306_TestAll();
    //     usleep(10000);
    // }
}

void Ssd1306TestDemo(void)
{
    osThreadAttr_t attr;

    attr.name = "Ssd1306Task";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 10240;
    attr.priority = osPriorityNormal;

    if (osThreadNew(Ssd1306TestTask, NULL, &attr) == NULL) {
        printf("[Ssd1306TestDemo] Falied to create Ssd1306TestTask!\n");
    }
}
APP_FEATURE_INIT(Ssd1306TestDemo);
