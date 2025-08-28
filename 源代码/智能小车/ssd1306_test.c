#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h> 
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "iot_gpio.h"
#include "hi_io.h"
#include "hi_time.h"
#include "ssd1306.h"
#include "iot_i2c.h"
#include "iot_watchdog.h"
#include "robot_control.h"
#include "iot_errno.h"

#define OLED_I2C_BAUDRATE 400*1000
#define GPIO13 13
#define GPIO14 14
#define FUNC_SDA 6
#define FUNC_SCL 6

extern int default_speed;

void Ssd1306TestTask(void* arg)
{
    (void) arg;
    hi_io_set_func(GPIO13, FUNC_SDA);
    hi_io_set_func(GPIO14, FUNC_SCL);
    IoTI2cInit(0, OLED_I2C_BAUDRATE);

    IoTWatchDogDisable();

    usleep(20*1000);
    ssd1306_Init();
    ssd1306_Fill(Black);
    ssd1306_SetCursor(0, 0);
    ssd1306_DrawString("Hello OpenHarmony!", Font_7x10, White);

    uint32_t start = HAL_GetTick();
    ssd1306_UpdateScreen();
    uint32_t end = HAL_GetTick();
    printf("ssd1306_UpdateScreen time cost: %d ms.\r\n", end - start);
    osDelay(100);
    char display_buf[32] = {0};

    while (1)
    {
        printf("g_car_status is %d\r\n",g_car_status);
        ssd1306_Fill(Black);
        ssd1306_SetCursor(0, 0);
        if(g_car_status == CAR_OBSTACLE_AVOIDANCE_STATUS) {
            
            ssd1306_DrawString("ultrasonic", Font_7x10, White);
            
        }
        else if (g_car_status == CAR_STOP_STATUS)
        {
            ssd1306_DrawString("Robot Car Stop", Font_7x10, White);
        }
        else if (g_car_status == CAR_TRACE_STATUS)
        {
            ssd1306_DrawString("trace", Font_7x10, White);
        }
         else if (g_car_status == CAR_REMOTE_CONTROL_STATUS) // <-- 添加新模式的显示
        {
        ssd1306_DrawString("Remote Control", Font_7x10, White);
        }
        // --- 第二行：显示速度 ---
        ssd1306_SetCursor(0, 12); // 移动到下一行
        snprintf(display_buf, sizeof(display_buf), "Speed: %u", default_speed);
        ssd1306_DrawString(display_buf, Font_7x10, White);

        // --- 第三行：显示行驶状态 ---
        ssd1306_SetCursor(0, 24); // 移动到再下一行
        switch (g_car_motion_status) {
            case MOTION_FORWARD:
                ssd1306_DrawString("State: Forward", Font_7x10, White);
                break;
            case MOTION_BACKWARD:
                ssd1306_DrawString("State: Backward", Font_7x10, White);
                break;
            case MOTION_LEFT:
                ssd1306_DrawString("State: Left", Font_7x10, White);
                break;
            case MOTION_RIGHT:
                ssd1306_DrawString("State: Right", Font_7x10, White);
                break;
            case MOTION_STOP:
            default:
                ssd1306_DrawString("State: Stop", Font_7x10, White);
                break;
        }
        ssd1306_UpdateScreen();
        osDelay(100);
    }
    
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
    attr.priority = 25;

    if (osThreadNew(Ssd1306TestTask, NULL, &attr) == NULL) {
        printf("[Ssd1306TestDemo] Falied to create Ssd1306TestTask!\n");
    }
}
APP_FEATURE_INIT(Ssd1306TestDemo);
