#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h> 
#include "wifi_connecter.h"
#include "net_params.h"

#include "ohos_init.h"
#include "cmsis_os2.h"
#include "iot_gpio.h"
#include "hi_io.h"
#include "hi_time.h"
#include "iot_watchdog.h"
#include "securec.h" 
#include "robot_control.h"
#include "hi_adc.h"
#include "iot_errno.h"
#include "robot_l9110s.c"

#define GPIO5 5
#define FUNC_GPIO 0
#define     ADC_TEST_LENGTH             (20)
#define     VLT_MIN                     (100)
#define     OLED_FALG_ON                ((unsigned char)0x01)
#define     OLED_FALG_OFF               ((unsigned char)0x00)

unsigned short  g_adc_buf[ADC_TEST_LENGTH] = { 0 };
unsigned short  g_gpio5_adc_buf[ADC_TEST_LENGTH] = { 0 };
unsigned int  g_gpio5_tick = 0;
unsigned char   g_car_control_mode = 0;
unsigned char   g_car_speed_control = 0;
unsigned int  g_car_control_demo_task_id = 0;
volatile CarStatus g_car_status = CAR_STOP_STATUS;
volatile CarMotionStatus g_car_motion_status = MOTION_STOP;
volatile unsigned int    g_current_speed = 0;

extern volatile float g_distance;

extern float GetDistance(void);
extern void trace_module(void);
extern void engine_turn_left(void);
extern void engine_turn_right(void);
extern void regress_middle(void);
extern void TcpServerTest(unsigned short port);
char key_flg = 0; 


void switch_init(void)
{
    IoTGpioInit(5);
    hi_io_set_func(5, 0);
    IoTGpioSetDir(5, IOT_GPIO_DIR_IN);
    hi_io_set_pull(5, 1);
}

//按键中断响应函数
void gpio5_isr_func_mode(void)
{
    printf("gpio5_isr_func_mode start\n");
    unsigned int tick_interval = 0;
    unsigned int current_gpio5_tick = 0; 

    current_gpio5_tick = hi_get_tick();
    tick_interval = current_gpio5_tick - g_gpio5_tick;
    
    if (tick_interval < KEY_INTERRUPT_PROTECT_TIME) {  
        return;
    }
    g_gpio5_tick = current_gpio5_tick;

    if (g_car_status == CAR_STOP_STATUS) {                
        g_car_status = CAR_TRACE_STATUS;                 // 寻迹        
        printf("trace\n");
    } else if (g_car_status == CAR_TRACE_STATUS) {       
        g_car_status = CAR_OBSTACLE_AVOIDANCE_STATUS;   // 超声波避障
        printf("ultrasonic\n");
    } else if (g_car_status == CAR_OBSTACLE_AVOIDANCE_STATUS) {                           
        g_car_status = CAR_REMOTE_CONTROL_STATUS;      // 远程控制
        printf("remote control\n");
    } else if (g_car_status == CAR_REMOTE_CONTROL_STATUS) {                           
        g_car_status = CAR_STOP_STATUS;                 // 停止
        printf("stop\n");
    }
}

void remote_control_module(void)
{
    // 1. 连接WiFi
    WifiDeviceConfig config = {0};
    strcpy_s(config.ssid, WIFI_MAX_SSID_LEN, PARAM_HOTSPOT_SSID);
    strcpy_s(config.preSharedKey, WIFI_MAX_KEY_LEN, PARAM_HOTSPOT_PSK);
    config.securityType = PARAM_HOTSPOT_TYPE;
    osDelay(10);
    int netId = ConnectToHotspot(&config);
    if (netId == -1) {
        printf("Connect to hotspot failed!\n");
        g_car_status = CAR_STOP_STATUS; // 如果连接失败，自动切回停止模式
        return;
    }
    printf("WiFi connected!\n");

    // 2. 启动TCP服务器 (它会阻塞直到模式改变)
    TcpServerTest(PARAM_SERVER_PORT);

    // 3. 当TcpServerTest返回后，说明模式已切换，断开WiFi
    printf("Exiting remote control mode, disconnecting wifi...\n");
    DisconnectWithHotspot(netId);
    car_stop(); // 确保退出时小车停止
}

void get_gpio5_voltage(char *param)
{
    int i;
    unsigned short data;
    unsigned int ret;
    unsigned short vlt;
    float voltage;
    float vlt_max = 0;
    float vlt_min = VLT_MIN;
    (void)param;

    hi_unref_param(param);
    memset_s(g_gpio5_adc_buf, sizeof(g_gpio5_adc_buf), 0x0, sizeof(g_gpio5_adc_buf));
    for (i = 0; i < ADC_TEST_LENGTH; i++) {
        ret = hi_adc_read(HI_ADC_CHANNEL_2, &data, HI_ADC_EQU_MODEL_4, HI_ADC_CUR_BAIS_DEFAULT, 0xF0); 
		//ADC_Channal_2  自动识别模式  CNcomment:4次平均算法模式 CNend */
        if (ret != IOT_SUCCESS) {
            printf("ADC Read Fail\n");
            return;
        }    
        g_gpio5_adc_buf[i] = data;
    }

    for (i = 0; i < ADC_TEST_LENGTH; i++) {  
        vlt = g_gpio5_adc_buf[i]; 
        voltage = (float)vlt * 1.8 * 4 / 4096.0;  
		/* vlt * 1.8* 4 / 4096.0为将码字转换为电压 */
        vlt_max = (voltage > vlt_max) ? voltage : vlt_max;
        vlt_min = (voltage < vlt_min) ? voltage : vlt_min;
    }
    printf("vlt_max is %f\r\n", vlt_max);
    if (vlt_max > 0.6 && vlt_max < 1.0) {
        gpio5_isr_func_mode();
    } 
}



//按键中断
void interrupt_monitor(void)
{
    unsigned int  ret = 0;
    /*gpio5 switch2 mode*/
    g_gpio5_tick = hi_get_tick();
    ret = IoTGpioRegisterIsrFunc(GPIO5, IOT_INT_TYPE_EDGE, IOT_GPIO_EDGE_FALL_LEVEL_LOW, get_gpio5_voltage, NULL);
    if (ret == IOT_SUCCESS) {
        printf(" register gpio5\r\n");
    }
}

/*Judge steering gear*/
static unsigned int engine_go_where(void)
{
    float left_distance = 0;
    float right_distance = 0;
    /*舵机往左转动测量左边障碍物的距离*/

    engine_turn_right();
    hi_sleep(100);
    left_distance = GetDistance();
    hi_sleep(100);

    /*归中*/
    regress_middle();
    hi_sleep(100);

    /*舵机往右转动测量右边障碍物的距离*/
    engine_turn_left();
    hi_sleep(100);
    right_distance = GetDistance();
    hi_sleep(100);

    /*归中*/
    regress_middle();
    if (left_distance > right_distance) {
        return CAR_TURN_LEFT;
    } else {
        return CAR_TURN_RIGHT;
    }
}

/*根据障碍物的距离来判断小车的行走方向
1、距离大于等于20cm继续前进
2、距离小于20cm，先停止再后退0.5s,再继续进行测距,再进行判断
*/
/*Judge the direction of the car*/
static void car_where_to_go(float distance)
{
    if (distance < DISTANCE_BETWEEN_CAR_AND_OBSTACLE) {
        car_stop();
        osDelay(50);
        car_backward(DEFAULT_CAR_SPEED);
        osDelay(50);
        car_stop();
        unsigned int ret = engine_go_where();
        printf("ret is %d\r\n", ret);
        if (ret == CAR_TURN_LEFT) {
            car_left(DEFAULT_CAR_SPEED);
            osDelay(125);
        } else if (ret == CAR_TURN_RIGHT) {
            car_right(DEFAULT_CAR_SPEED);
            osDelay(125);
        }
        car_stop();
    } else {
        car_forward(DEFAULT_CAR_SPEED);
    } 
}

/*car mode control func*/
static void car_mode_control_func(void)
{
    regress_middle();
    while (g_car_status == CAR_OBSTACLE_AVOIDANCE_STATUS) {
        float m_distance = g_distance;     // 读取后台任务测得值
        car_where_to_go(m_distance);
        osDelay(20);                       // 让出 CPU，避免饥饿
    }
    printf("car_mode_control_func: 模式已切换\n");
    regress_middle();
}

void RobotCarTestTask(void* param)
{
    robot_init();

    (void)param;
	printf("switch\r\n");
    switch_init();
    interrupt_monitor();

    while (1) {
        switch (g_car_status) {
            case CAR_STOP_STATUS:
                car_stop();
                break;
            case CAR_OBSTACLE_AVOIDANCE_STATUS:
                car_mode_control_func();
                break;
            case CAR_TRACE_STATUS:
                trace_module();
                break;
            case CAR_REMOTE_CONTROL_STATUS: // <-- 添加新模式的处理
                remote_control_module();
                break;
            default:
                break;
        }
        IoTWatchDogDisable();
        osDelay(5);        
    }
}

void RobotCarDemo(void)
{
    osThreadAttr_t attr;

    attr.name = "RobotCarTestTask";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 10240;
    attr.priority = 25;

    if (osThreadNew(RobotCarTestTask, NULL, &attr) == NULL) {
        printf("[Ssd1306TestDemo] Falied to create RobotCarTestTask!\n");
    }
}
APP_FEATURE_INIT(RobotCarDemo);