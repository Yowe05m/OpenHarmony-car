#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "robot_control.h"

#include "ohos_init.h"
#include "cmsis_os2.h"
#include "iot_gpio.h"
#include "hi_io.h"
#include "hi_time.h"
#include "iot_pwm.h"
#include "hi_pwm.h"

// 左轮电机引脚
#define M_LEFT_IA HI_IO_NAME_GPIO_0 // IA -> PWM3
#define M_LEFT_IB HI_IO_NAME_GPIO_1 // IB -> PWM4
// 右轮电机引脚
#define M_RIGHT_IA HI_IO_NAME_GPIO_9  // IA -> PWM0
#define M_RIGHT_IB HI_IO_NAME_GPIO_10 // IB -> PWM1

// PWM端口号
#define PWM_PORT_LEFT_IA  HI_PWM_PORT_PWM3
#define PWM_PORT_LEFT_IB  HI_PWM_PORT_PWM4
#define PWM_PORT_RIGHT_IA HI_PWM_PORT_PWM0
#define PWM_PORT_RIGHT_IB HI_PWM_PORT_PWM1

#define PWM_FREQ 8000 // PWM频率设置为8KHz, 在推荐范围内

// 底层函数：设置单个电机的速度
static void set_single_motor_speed(unsigned int ia_gpio, unsigned int ib_gpio,
                                   unsigned int ia_pwm_port,  unsigned int ib_pwm_port,
                                   int speed)
{
    if (speed > 100)  speed = 100;
    if (speed < -100) speed = -100;

    if (speed > 0) { // 正转
        IoTPwmStop(ib_pwm_port);
        hi_io_set_func(ib_gpio, HI_IO_FUNC_GPIO_1_GPIO); // IB引脚设为GPIO输出低电平
        IoTGpioSetDir(ib_gpio, IOT_GPIO_DIR_OUT);
        IoTGpioSetOutputVal(ib_gpio, IOT_GPIO_VALUE0);

        hi_io_set_func(ia_gpio, HI_IO_FUNC_GPIO_0_PWM3_OUT); // IA引脚设为PWM输出
        IoTPwmStart(ia_pwm_port, speed, PWM_FREQ);
    } else if (speed < 0) { // 反转
        IoTPwmStop(ia_pwm_port);
        hi_io_set_func(ia_gpio, HI_IO_FUNC_GPIO_0_GPIO); // IA引脚设为GPIO输出低电平
        IoTGpioSetDir(ia_gpio, IOT_GPIO_DIR_OUT);
        IoTGpioSetOutputVal(ia_gpio, IOT_GPIO_VALUE0);
        
        hi_io_set_func(ib_gpio, HI_IO_FUNC_GPIO_1_PWM4_OUT); // IB引脚设为PWM输出
        IoTPwmStart(ib_pwm_port, -speed, PWM_FREQ);
    } else { // 速度为0，刹车
        IoTPwmStop(ia_pwm_port);
        IoTPwmStop(ib_pwm_port);
        hi_io_set_func(ia_gpio, HI_IO_FUNC_GPIO_0_GPIO);
        IoTGpioSetDir(ia_gpio, IOT_GPIO_DIR_OUT);
        IoTGpioSetOutputVal(ia_gpio, IOT_GPIO_VALUE1); // IA, IB均为高电平实现刹车
        
        hi_io_set_func(ib_gpio, HI_IO_FUNC_GPIO_1_GPIO);
        IoTGpioSetDir(ib_gpio, IOT_GPIO_DIR_OUT);
        IoTGpioSetOutputVal(ib_gpio, IOT_GPIO_VALUE1);
    }
}

// 公共接口：设置整车速度
void set_car_speed(int left_speed, int right_speed)
{
    set_single_motor_speed(M_LEFT_IA, M_LEFT_IB, PWM_PORT_LEFT_IA, PWM_PORT_LEFT_IB, left_speed);
    set_single_motor_speed(M_RIGHT_IA, M_RIGHT_IB, PWM_PORT_RIGHT_IA, PWM_PORT_RIGHT_IB, right_speed);
}

// 初始化所有电机相关的GPIO和PWM
void robot_init(void) {
    // 初始化左轮的两个引脚为GPIO输出
    IoTGpioInit(M_LEFT_IA);
    hi_io_set_func(M_LEFT_IA, HI_IO_FUNC_GPIO_0_GPIO);
    IoTGpioSetDir(M_LEFT_IA, IOT_GPIO_DIR_OUT);

    IoTGpioInit(M_LEFT_IB);
    hi_io_set_func(M_LEFT_IB, HI_IO_FUNC_GPIO_1_GPIO);
    IoTGpioSetDir(M_LEFT_IB, IOT_GPIO_DIR_OUT);

    // 初始化右轮的两个引脚为GPIO输出
    IoTGpioInit(M_RIGHT_IA);
    hi_io_set_func(M_RIGHT_IA, HI_IO_FUNC_GPIO_9_GPIO);
    IoTGpioSetDir(M_RIGHT_IA, IOT_GPIO_DIR_OUT);

    IoTGpioInit(M_RIGHT_IB);
    hi_io_set_func(M_RIGHT_IB, HI_IO_FUNC_GPIO_10_GPIO);
    IoTGpioSetDir(M_RIGHT_IB, IOT_GPIO_DIR_OUT);
    
    // 初始化所有可能用到的PWM端口
    IoTPwmInit(PWM_PORT_LEFT_IA);
    IoTPwmInit(PWM_PORT_LEFT_IB);
    IoTPwmInit(PWM_PORT_RIGHT_IA);
    IoTPwmInit(PWM_PORT_RIGHT_IB);
}

// 停止所有PWM输出, 并将引脚设为刹车状态
void car_stop(void) {
    set_car_speed(0, 0);
    g_car_motion_status = MOTION_STOP;
    g_current_speed = 0;
}


// 小车前进 (速度: 0-100)
void car_forward(unsigned int speed) {
    set_car_speed(speed, speed);
    g_car_motion_status = MOTION_FORWARD;
    g_current_speed = speed;
}

// 小车后退 (速度: 0-100)
void car_backward(unsigned int speed) {
    set_car_speed(-speed, -speed);
    g_car_motion_status = MOTION_BACKWARD;
    g_current_speed = speed;
}

// 小车左转 (速度: 0-100)
void car_left(unsigned int speed) {
    set_car_speed(0, speed); // 原地左转
    g_car_motion_status = MOTION_LEFT;
    g_current_speed = speed;
}

// 小车右转 (速度: 0-100)
void car_right(unsigned int speed) {
    set_car_speed(speed, 0); // 原地右转
    g_car_motion_status = MOTION_RIGHT;
    g_current_speed = speed;
}