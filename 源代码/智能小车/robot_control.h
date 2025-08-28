#ifndef ROBOT_CONTROL_H__
#define ROBOT_CONTROL_H__

#define     CAR_CONTROL_DEMO_TASK_STAK_SIZE   (1024*10)
#define     CAR_CONTROL_DEMO_TASK_PRIORITY    (25)
#define     DISTANCE_BETWEEN_CAR_AND_OBSTACLE (20)
#define     DEFAULT_CAR_SPEED                 (60)
#define     KEY_INTERRUPT_PROTECT_TIME        (30)
#define     CAR_TURN_LEFT                     (0)
#define     CAR_TURN_RIGHT                    (1)

// 小车状态枚举
typedef enum {
    CAR_STOP_STATUS = 0,
    CAR_OBSTACLE_AVOIDANCE_STATUS, 
    CAR_TRACE_STATUS,
    CAR_REMOTE_CONTROL_STATUS
} CarStatus;

// --- 全局共享变量 ---
// 使用 extern 和 volatile 声明，确保多线程安全访问
// volatile 防止编译器过度优化，确保每次都从内存读取最新值

// 定义具体的车辆运动状态
typedef enum {
    MOTION_STOP = 0,
    MOTION_FORWARD,
    MOTION_BACKWARD,
    MOTION_LEFT,
    MOTION_RIGHT
} CarMotionStatus;

extern volatile CarStatus g_car_status;
extern volatile CarMotionStatus g_car_motion_status;
extern volatile unsigned int    g_current_speed;


// --- 函数声明 ---
void switch_init(void);
void interrupt_monitor(void);

#endif // ROBOT_CONTROL_H__