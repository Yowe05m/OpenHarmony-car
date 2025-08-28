#include "ohos_init.h"
#include "cmsis_os2.h"
#include "iot_gpio.h"
#include "hi_io.h"
#include "hi_time.h"
#include "iot_watchdog.h"
#include "robot_control.h"
#include "iot_errno.h"
#include "hi_pwm.h"
#include "hi_timer.h"
#include "iot_pwm.h"
#include <stdio.h>
#include "securec.h"

#include "hi_adc.h"
#include "hi_types_base.h"

#include <stdbool.h>

// 左右两轮电机各由一个L9110S驱动
// GPOI0和GPIO1控制左轮,GPIO9和GPIO10控制右轮。通过输入GPIO的电平高低控制车轮正转/反转/停止/刹车。
#define GPIO0 0
#define GPIO1 1
#define GPIO9 9
#define GPIO10 10

#define M1_IA_PWM 3
#define M1_IB_PWM 4
#define M2_IA_PWM 0
#define M2_IB_PWM 1

// 查阅机器人板原理图可知
// 左边的红外传感器通过GPIO11与3861芯片连接
// 右边的红外传感器通过GPIO12与3861芯片连接
#define GPIO11 11
#define GPIO12 12

#define GPIO_FUNC 0
#define GPIO_FUNC_PWM 5
#define PWM_FREQ 8000 // 8kHz

// —— ADC 按键 S2 定义（同 adc_led.c） ——
#define ADC_CHANNEL HI_ADC_CHANNEL_2
#define ADC_SAMPLE_COUNT 64
#define VLT_MIN_INIT 100

#define KEY_NONE 0
#define KEY_S1 1
#define KEY_S2 2
#define KEY_S3 3

extern void car_backward(unsigned int speed);
extern void car_forward(unsigned int speed);
extern void car_left(unsigned int speed);
extern void car_right(unsigned int speed);
extern void car_stop(void);

typedef enum
{
    TRACE_NORMAL,        // 正常巡线
    TRACE_CROSS_FORWARD, // 探测到左右同时黑，前行短距用于区分交叉口/终点
    TRACE_CROSS_CHECK,   // 前行后复查：判定交叉口还是终点
    TRACE_END,           // 终点：停车
    TRACE_SHARP_FORWARD, // 探测到单边黑，前行短距进入急转弯
    TRACE_SHARP_TURN,    // 急转弯阶段：持续转向
    TRACE_SHARP_ADJUST   // 急转弯结束后微调
} TraceState;

static TraceState trace_state = TRACE_NORMAL;
static int state_counter = 0;        // 毫秒级计数
static bool sharp_left_turn = false; // 记录急转弯方向：true=左侧先黑，做左急转

// 延时参数
#define CROSS_FORWARD_DELAY_MS 12 // 遇横线，停止时间（双黑，停止）
#define SHARP_FORWARD_DELAY_MS 12 // 单侧黑线，继续前行时间
#define SHARP_ADJUST_DELAY_MS 30  // 转弯后，调整时间

static hi_u16 g_adc_buf[ADC_SAMPLE_COUNT];
static int key_status = KEY_NONE;
extern char key_flg;

int default_speed = 60;

static int g_car_speed_left = 0;
static int g_car_speed_right = 0;

volatile float g_distance = 0.0f;

// 声明超声波测距函数
extern float GetDistance(void);

static void init_pwm_channel(unsigned int pwm_port)
{
    IoTPwmInit(pwm_port);
}

// 全车速度接口
static void set_car_speed(int left_speed, int right_speed)
{
    if (left_speed > 0 && right_speed == 0)
    {
        car_right(left_speed);
    }
    else if (left_speed == 0 && right_speed > 0)
    {
        car_left(right_speed);
    }
    else if (left_speed == 0 && right_speed == 0)
    {
        car_stop();
    }
    else
    {
        car_forward(default_speed);
    }
}

// —— ADC 采样 & 按键识别 ——
static void convert_to_voltage(hi_u32 len)
{
    float vlt_max = 0.0f, vlt_min = VLT_MIN_INIT, vlt;
    for (hi_u32 i = 0; i < len; i++)
    {
        float voltage = (float)g_adc_buf[i] * 1.8f * 4.0f / 4096.0f;
        if (voltage > vlt_max)
            vlt_max = voltage;
        if (voltage < vlt_min)
            vlt_min = voltage;
    }
    vlt = (vlt_max + vlt_min) / 2.0f;
    if ((vlt > 1.0f && vlt < 1.5f) && !key_flg)
    {
        key_flg = 1;
        key_status = KEY_S2;
    }
    else if (vlt > 3.0f)
    {
        key_flg = 0;
    }
}

static void sample_adc_key(void)
{
    hi_u16 raw;
    memset_s(g_adc_buf, sizeof(g_adc_buf), 0, sizeof(g_adc_buf));
    for (int i = 0; i < ADC_SAMPLE_COUNT; i++)
    {
        if (hi_adc_read(ADC_CHANNEL, &raw,
                        HI_ADC_EQU_MODEL_1,
                        HI_ADC_CUR_BAIS_DEFAULT, 0) != HI_ERR_SUCCESS)
        {
            return;
        }
        g_adc_buf[i] = raw;
    }
    convert_to_voltage(ADC_SAMPLE_COUNT);
}

static int get_key_event(void)
{
    int e = key_status;
    key_status = KEY_NONE;
    return e;
}

// —— SpeedKeyTask：轮询 S2，更新 default_speed ——
static void SpeedKeyTask(void *arg)
{
    (void)arg;
    // 初始化 ADC 引脚复用
    // 切换到 GPIO 功能
    hi_io_set_func(HI_IO_NAME_GPIO_5, HI_IO_FUNC_GPIO_5_GPIO);
    // 打开上拉
    hi_io_set_pull(HI_IO_NAME_GPIO_5, HI_IO_PULL_UP);
    // 再用 IOT 层初始化 GPIO5 并设为输入
    IoTGpioInit(5);
    IoTGpioSetDir(5, IOT_GPIO_DIR_IN);

    while (1)
    {
        sample_adc_key();
        if (get_key_event() == KEY_S2)
        {

            if (trace_state == TRACE_END)
            {
                trace_state = TRACE_NORMAL; // 恢复正常巡线状态
                continue;
            }

            default_speed += 10;
            if (default_speed > 100)
                default_speed = 60;
            printf("New default_speed = %d\n", default_speed);
        }
        osDelay(5);
    }
}

// 普通线程里做巡线+遇障停车逻辑
static void TraceTask(void *arg)
{
    (void)arg;
    for (;;)
    {
        // 等待进 trace 模式
        while (g_car_status != CAR_TRACE_STATUS)
        {
            osDelay(5);
        }

        // 模式进入瞬间重置状态机
        trace_state = TRACE_NORMAL;
        state_counter = 0;
        sharp_left_turn = false;

        // 进入 trace 模式后循环
        while (g_car_status == CAR_TRACE_STATUS)
        {
            // ———— (A) 超声波遇障停车 ————
            if (g_distance < DISTANCE_BETWEEN_CAR_AND_OBSTACLE)
            {
                set_car_speed(0, 0);
                osDelay(5);
                continue; // 跳过红外
            }

            // 1) 读取左右传感器电平
            IotGpioValue io_left, io_right;
            IoTGpioGetInputVal(GPIO11, &io_left);
            IoTGpioGetInputVal(GPIO12, &io_right);

            // IOT_GPIO_VALUE0 表示检测到黑色
            bool left_black = (io_left == IOT_GPIO_VALUE0);
            bool right_black = (io_right == IOT_GPIO_VALUE0);

            // 2) 状态机
            int spd_l = default_speed;
            int spd_r = default_speed;

            switch (trace_state)
            {
            case TRACE_NORMAL:
                if (left_black && right_black)
                {
                    // 同时检测到黑：可能交叉口或终点
                    trace_state = TRACE_CROSS_FORWARD;
                    state_counter = 0;
                    // 前行短距：维持前进
                    spd_l = spd_r = default_speed;
                }
                else if (left_black && !right_black)
                {
                    // 仅左检测到黑：急转弯开始
                    sharp_left_turn = true;
                    trace_state = TRACE_SHARP_FORWARD;
                    state_counter = 0;
                    spd_l = spd_r = default_speed;
                }
                else if (right_black && !left_black)
                {
                    // 仅右检测到黑：急转弯开始
                    sharp_left_turn = false;
                    trace_state = TRACE_SHARP_FORWARD;
                    state_counter = 0;
                    spd_l = spd_r = default_speed;
                }
                else
                {
                    // 白线：正常前进
                    spd_l = spd_r = default_speed;
                }
                break;

            case TRACE_CROSS_FORWARD:
                // 前行一段时间，再做复查

                if (left_black && !right_black)
                {
                    // 仅左检测到黑：急转弯开始
                    sharp_left_turn = true;
                    trace_state = TRACE_SHARP_FORWARD;
                    state_counter = 0;
                    spd_l = spd_r = default_speed;
                    break;
                }
                else if (right_black && !left_black)
                {
                    // 仅右检测到黑：急转弯开始
                    sharp_left_turn = false;
                    trace_state = TRACE_SHARP_FORWARD;
                    state_counter = 0;
                    spd_l = spd_r = default_speed;
                    break;
                }

                if (state_counter < CROSS_FORWARD_DELAY_MS)
                {
                    spd_l = spd_r = default_speed;
                    state_counter++;
                }
                else
                {
                    trace_state = TRACE_CROSS_CHECK;
                    state_counter = 0;
                }
                break;

            case TRACE_CROSS_CHECK:
                if (left_black && right_black)
                {
                    // 复查依然是宽线：到达终点
                    trace_state = TRACE_END;
                    spd_l = spd_r = 0;
                }
                else if (!left_black && !right_black)
                {
                    // 复查全白：经过交叉口，继续正常巡线
                    trace_state = TRACE_NORMAL;
                    spd_l = spd_r = default_speed;
                }
                else
                {
                    // 复查单边：实际是一个弯道，进入急转弯逻辑
                    sharp_left_turn = left_black;
                    trace_state = TRACE_SHARP_FORWARD;
                    state_counter = 0;
                    spd_l = spd_r = default_speed;
                }
                break;

            case TRACE_END:
                // 终点一直停车
                spd_l = spd_r = 0;
                break;

            case TRACE_SHARP_FORWARD:
                // 一侧黑线，前行短距，进入急转弯
                if (left_black && right_black)
                {
                    // 同时检测到黑：可能交叉口或终点
                    trace_state = TRACE_CROSS_FORWARD;
                    // 前行短距：维持前进
                    spd_l = spd_r = default_speed;
                }
                // 急转弯前行短距，保证车头完全驶入弯道
                else if (state_counter < SHARP_FORWARD_DELAY_MS)
                {
                    spd_l = spd_r = default_speed;
                    state_counter++;
                }
                else
                {
                    trace_state = TRACE_SHARP_TURN;
                    state_counter = 0;
                }
                break;

            case TRACE_SHARP_TURN:
                // 持续转向
                if (sharp_left_turn)
                {
                    spd_l = 0;
                    spd_r = default_speed;
                }
                else
                {
                    spd_l = default_speed;
                    spd_r = 0;
                }
                // 只有急转方向的另一侧检测到黑线，才结束转弯
                if ((sharp_left_turn && right_black) || (!sharp_left_turn && left_black))
                {
                    trace_state = TRACE_SHARP_ADJUST;
                    state_counter = 0;
                }
                break;

            case TRACE_SHARP_ADJUST:
                // 急转弯后微调

                if (left_black && right_black)
                {
                    // 同时检测到黑：可能交叉口或终点
                    trace_state = TRACE_CROSS_FORWARD;
                    state_counter = 0;
                    // 前行短距：维持前进
                    spd_l = spd_r = default_speed;
                }
                else if (state_counter < SHARP_ADJUST_DELAY_MS)
                {
                    // 微调：相反轮短时转动以校正姿态 {
                    if (sharp_left_turn)
                    {
                        if (left_black && !right_black)
                        {
                            // 仅左检测到黑：急转弯开始
                            sharp_left_turn = true;
                            trace_state = TRACE_SHARP_FORWARD;
                            state_counter = 0;
                            spd_l = spd_r = default_speed;
                            break;
                        }

                        spd_l = default_speed;
                        spd_r = 0;
                    }
                    else
                    {
                        if (right_black && !left_black)
                        {
                            // 仅右检测到黑：急转弯开始
                            sharp_left_turn = false;
                            trace_state = TRACE_SHARP_FORWARD;
                            state_counter = 0;
                            spd_l = spd_r = default_speed;
                            break;
                        }

                        spd_l = 0;
                        spd_r = default_speed;
                    }
                    state_counter++;
                }
                else
                {
                    // 调整完成，返回正常巡线
                    trace_state = TRACE_NORMAL;
                }
                break;
            }

            // 3) 更新全局速度，由主循环调用 set_car_speed
            g_car_speed_left = spd_l;
            g_car_speed_right = spd_r;

            set_car_speed(g_car_speed_left, g_car_speed_right);
            osDelay(1); // 延时，避免过快循环
        }
        // 走到这里说明模式退出，循环回去等待下一次进入
    }
}

// 普通线程里做测距 —— 避免在回调/中断上下文里调用 GetDistance()
static void UltrasonicTask(void *arg)
{
    (void)arg;
    while (1)
    {
        // 这是在任务上下文里调用，不会触发 LOS_SemPend 警告
        g_distance = GetDistance(); // 来自 robot_hcsr04.c :contentReference[oaicite:0]{index=0}
        osDelay(50);
    }
}

void trace_module(void)
{
    static bool inited = false;
    if (!inited)
    {
        inited = true;

        // 1. 初始化所有 PWM 通道（只需一次）
        init_pwm_channel(M1_IA_PWM);
        init_pwm_channel(M1_IB_PWM);
        init_pwm_channel(M2_IA_PWM);
        init_pwm_channel(M2_IB_PWM);

        // 2. 创建超声、按键、巡线任务（只需一次）
        osThreadAttr_t attr;
        attr.priority = 25;

        attr.name = "UltrasonicTask";
        attr.stack_size = 4096;
        if (osThreadNew(UltrasonicTask, NULL, &attr) == NULL)
        {
            printf("Failed to create UltrasonicTask\n");
        }

        attr.name = "SpeedKeyTask";
        attr.stack_size = 2048;
        if (osThreadNew(SpeedKeyTask, NULL, &attr) == NULL)
        {
            printf("Failed to create SpeedKeyTask\n");
        }

        attr.name = "TraceTask";
        attr.stack_size = 4096;
        if (osThreadNew(TraceTask, NULL, &attr) == NULL)
        {
            printf("Failed to create TraceTask\n");
        }

        printf("trace_module initialized\n");
    }

    // 3) 主循环仅等待模式退出
    // while (g_car_status == CAR_TRACE_STATUS) {
    //    osDelay(5);
    //}
    // **立刻返回**，由 TraceTask 自身根据 g_car_status 切换执行/挂起
}