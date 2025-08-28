#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <hi_types_base.h>
#include <hi_io.h>
#include <hi_gpio.h>
#include <hi_adc.h>
#include <hi_stdlib.h>
#include <hi_task.h>

#include "ohos_init.h"
#include "cmsis_os2.h"

#define LED_GPIO_PIN       9                /* 板载 LED 连接在 GPIO9 */
#define ADC_CHANNEL        HI_ADC_CHANNEL_2  /* 多键接在 ADC2 上 */
#define ADC_SAMPLE_COUNT   64               /* 采样 64 次做滤波 */
#define VLT_MIN_INIT       100              /* 初始最小电压阈值 */

#define KEY_NONE           0
#define KEY_S1             1  /* 打开 LED */
#define KEY_S2             2  /* 关闭 LED */
#define KEY_S3             3  /* 切换 LED */

static hi_u16  g_adc_buf[ADC_SAMPLE_COUNT];
static int     key_status = KEY_NONE;
static char    key_flg    = 0;
static char    led_state  = 0;   /* 0=关，1=开 */

static int get_key_event(void)
{
    int e = key_status;
    key_status = KEY_NONE;
    return e;
}

/* 把 ADC 原始码转为电压并判定是哪键 */
static void convert_to_voltage(hi_u32 len)
{
    float vlt_max = 0.0f, vlt_min = VLT_MIN_INIT, vlt;
    for (hi_u32 i = 0; i < len; i++) {
        float voltage = (float)g_adc_buf[i] * 1.8f * 4.0f / 4096.0f;
        if (voltage > vlt_max) vlt_max = voltage;
        if (voltage < vlt_min) vlt_min = voltage;
    }
    vlt = (vlt_max + vlt_min) / 2.0f;

    if ((vlt > 0.4f && vlt < 0.6f) && !key_flg) {
        key_flg    = 1; 
        key_status = KEY_S1;
    } 
    else if ((vlt > 0.8f && vlt < 1.1f) && !key_flg) {
        key_flg    = 1; 
        key_status = KEY_S2;
    } 
    else if ((vlt > 0.01f && vlt < 0.3f) && !key_flg) {
        key_flg    = 1; 
        key_status = KEY_S3;
    } 
    else if (vlt > 3.0f) {
        /* 松开时复位，允许下一次检测 */
        key_flg = 0;
    }
}

/* 连续读 ADC_SAMPLE_COUNT 次 ADC，并更新 key_status */
static void sample_adc_key(void)
{
    hi_u16 raw;
    memset_s(g_adc_buf, sizeof(g_adc_buf), 0, sizeof(g_adc_buf));
    
    for (int i = 0; i < ADC_SAMPLE_COUNT; i++) {
        if (hi_adc_read(ADC_CHANNEL, &raw,
                        HI_ADC_EQU_MODEL_1,
                        HI_ADC_CUR_BAIS_DEFAULT, 0) != HI_ERR_SUCCESS) {
            printf("ADC read failed\n");
            return;
        }
        g_adc_buf[i] = raw;
    }
    convert_to_voltage(ADC_SAMPLE_COUNT);
}

/* 主任务：检测按键，控制 LED 并在串口输出状态 */
static void LedTask(void *arg)
{
    (void)arg;

    /* 1. 初始化 LED GPIO */
    hi_gpio_init();
    hi_io_set_func(HI_IO_NAME_GPIO_9, HI_IO_FUNC_GPIO_9_GPIO);
    hi_gpio_set_dir(HI_GPIO_IDX_9, HI_GPIO_DIR_OUT);
    /* 默认为关灯（GPIO 输出高电平）*/
    hi_gpio_set_ouput_val(HI_GPIO_IDX_9, HI_GPIO_VALUE1);
    led_state = 0;

    hi_io_set_func(HI_IO_NAME_GPIO_5, HI_IO_FUNC_GPIO_5_GPIO);

    while (1) {
        sample_adc_key();
        int evt = get_key_event();
        if (evt == KEY_S1) {
            if (!led_state) {
                hi_gpio_set_ouput_val(HI_GPIO_IDX_9, HI_GPIO_VALUE0);
                led_state = 1;
                printf("LED ON\n");
            }
        } else if (evt == KEY_S2) {
            if (led_state) {
                hi_gpio_set_ouput_val(HI_GPIO_IDX_9, HI_GPIO_VALUE1);
                led_state = 0;
                printf("LED OFF\n");
            }
        } else if (evt == KEY_S3) {
            /* 切换状态 */
            if (led_state) {
                hi_gpio_set_ouput_val(HI_GPIO_IDX_9, HI_GPIO_VALUE1);
                led_state = 0;
                printf("LED OFF\n");
            } else {
                hi_gpio_set_ouput_val(HI_GPIO_IDX_9, HI_GPIO_VALUE0);
                led_state = 1;
                printf("LED ON\n");
            }
        }
        // 50ms 延时
        osDelay(50);
    }
}

void adc_led(void)
{
    osThreadAttr_t attr = {
        .name       = "ADC_LED_Task",
        .stack_size = 2048,
        .priority   = 26,
    };
    if (osThreadNew(LedTask, NULL, &attr) == NULL) {
        printf("Failed to create ADC_LED_TASK\n");
    }
}
SYS_RUN(adc_led);
