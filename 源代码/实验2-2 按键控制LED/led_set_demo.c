#include <unistd.h>
#include "stdio.h"
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "iot_gpio.h"
#include "hi_io.h"

// LED 接 GPIO9，按键 S2 接 GPIO5
#define LED_GPIO 9 
#define KEY_GPIO 5

static void LedTask(void *arg)
{
    (void)arg;
    
    // ---- 初始化 LED 引脚 ----
    IoTGpioInit(LED_GPIO);
    // 设置复用为 GPIO9 模式
    hi_io_set_func(HI_IO_NAME_GPIO_9, HI_IO_FUNC_GPIO_9_GPIO); 
    IoTGpioSetDir(LED_GPIO, IOT_GPIO_DIR_OUT);

    // ---- 初始化 按键 引脚 ----
    IoTGpioInit(KEY_GPIO);
    // 设置复用为 GPIO5 模式
    hi_io_set_func(HI_IO_NAME_GPIO_5, HI_IO_FUNC_GPIO_5_GPIO); 
    // 打开内部上拉，保证按下前为高电平
    hi_io_set_pull(HI_IO_NAME_GPIO_5, HI_IO_PULL_UP); 
    // 启用输入，CPU 才能读取到电平变化
    hi_io_set_input_enable(HI_IO_NAME_GPIO_5, HI_TRUE); 
    // 使用斯密特触发以做简单去抖
    hi_io_set_schmitt(HI_IO_NAME_GPIO_5, HI_TRUE); 
    IoTGpioSetDir(KEY_GPIO, IOT_GPIO_DIR_IN);

    while (1)
    {
        IotGpioValue keyVal;
        // 读取按键状态
        IoTGpioGetInputVal(KEY_GPIO, &keyVal);

        if (keyVal == IOT_GPIO_VALUE0) {
            // 按下时，点亮 LED，打印 1
            IoTGpioSetOutputVal(LED_GPIO, IOT_GPIO_VALUE0);
            printf("HI_GPIO_VALUE_1\n");
        } else {
            // 松开时，熄灭 LED，打印 0
            IoTGpioSetOutputVal(LED_GPIO, IOT_GPIO_VALUE1);
            printf("HI_GPIO_VALUE_0\n");
        }
        // 延迟 50ms，防抖＋减小打印频率
        osDelay(50);
    }
}

void led_set_demo(void)
{
    osThreadAttr_t attr;
    attr.name = "LedTask";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 1024;
    attr.priority = 26;
    if (osThreadNew((osThreadFunc_t)LedTask, NULL, &attr) == NULL) {
        printf("[LedExample] Falied to create LedTask!\n");
    }
}

SYS_RUN(led_set_demo);