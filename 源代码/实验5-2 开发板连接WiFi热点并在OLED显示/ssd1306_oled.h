#ifndef SSD1306_OLED_H
#define SSD1306_OLED_H

#include <stdint.h>

// 调用一次，完成 I²C 引脚配置 + OLED 初始化 + 清屏
void Oled_Init(void);

// 在 (x,y) 坐标开始写字符串并刷新屏幕
void Oled_ShowString(uint8_t x, uint8_t y, const char *str);

#endif  // SSD1306_OLED_H
