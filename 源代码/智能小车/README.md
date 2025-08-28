robot_control.c /  robot_control.h - 中央控制器与模式调度器，负责管理小车的顶层状态，根据按键S1输入调度切换到不同的工作模式，并调用相应的接口。

trace_model.c - 循迹模式，以及给全局g_distance赋值。

car_tcp_server_test.c - 远程控制模式，让小车接受并解析来自win端的命令。

wifi_connecter.c / wifi_connecter.h - 封装WiFi连接功能，供远程控制模式使用。

net_*.h 系列头文件 - 网络配置与工具集，这一组文件为网络功能提供了配置参数和通用工具。

net_params.h: 网络参数配置文件，定义了所有网络相关的常量，如WiFi热点的SSID、密码，以及TCP服务器的端口号。

net_demo.h: 测试框架宏，定义了一个 SERVER_TEST_DEMO 宏，用于快速生成一个标准的服务器测试入口。

net_common.h: 网络编程兼容性头文件，通过预处理指令判断当前编译环境，并包含正确的socket头文件。

robot_hcsr04.c: 封装超声波测距函数。

robot_l9110s.c: 封装电机控制函数，提供控制小车速度的函数接口。

robot_sg90.c: 封装舵机转向函数，提供接口。

ssd1306_test.c: 封装OLED显示功能，在这里进行小车工作模式、速度、运动状态在OLED的显示。