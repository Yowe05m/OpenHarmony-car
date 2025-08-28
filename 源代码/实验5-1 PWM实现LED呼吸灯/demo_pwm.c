#include <stdio.h>
#include "ohos_init.h"      // 系统启动宏
#include "cmsis_os2.h"      // RTOS2 接口
#include "iot_gpio.h"       // HAL: GPIO 接口
#include "hi_io.h"          // SDK: IO 复用、上拉
#include "iot_pwm.h"        // HAL: PWM 接口
#include "hi_pwm.h"         // SDK: PWM 接口    

#define LED_GPIO_PIN        HI_IO_NAME_GPIO_9
#define PWM_PORT            HI_PWM_PORT_PWM0
#define PWM_FREQ            4000    // 频率 4kHz（>=2442Hz）
#define MAX_DUTY_PERCENT    99      // HAL 占空比最大 99%
#define DUTY_STEP 1                 // 每次改变 1%

static void PwmBreathingTask(void *arg)
{
    (void)arg;
    // 1. 初始化 GPIO
    IoTGpioInit(LED_GPIO_PIN);
    hi_io_set_func(LED_GPIO_PIN, HI_IO_FUNC_GPIO_9_PWM0_OUT);    // 复用为 PWM0
    IoTGpioSetDir(LED_GPIO_PIN, IOT_GPIO_DIR_OUT);

    // 2. 初始化 PWM 模块
    IoTPwmInit(PWM_PORT);

    // 3. 呼吸灯循环
    while (1) {
        for (int d = 1; d <= 30; d+= DUTY_STEP) {
            IoTPwmStart(PWM_PORT, d, PWM_FREQ);
            osDelay(5);
        }
        for (int d = 31; d <= MAX_DUTY_PERCENT; d+= (DUTY_STEP+3)) {
            IoTPwmStart(PWM_PORT, d, PWM_FREQ);
            osDelay(5);
        }
        for (int d = MAX_DUTY_PERCENT; d >= 31; d-= (DUTY_STEP+3)) {
            IoTPwmStart(PWM_PORT, d, PWM_FREQ);
            osDelay(5);
        }
        for (int d = 30; d >= 1; d-= DUTY_STEP) {
            IoTPwmStart(PWM_PORT, d, PWM_FREQ);
            osDelay(5);
        }
    }
}

// 入口：创建线程
static void PwmBreathingEntry(void)
{
    osThreadAttr_t attr = {
        .name       = "PwmBreathing", 
        .stack_size = 2048, 
        .priority   = osPriorityNormal
    };
    if (osThreadNew(PwmBreathingTask, NULL, &attr) == NULL) {
        printf("Failed to create PwmBreathingTask!\n");
    }
}
SYS_RUN(PwmBreathingEntry);
