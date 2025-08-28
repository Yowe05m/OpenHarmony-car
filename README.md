```
OpenHarmony-Car/
 ┣ 源代码
 ┃ ┣ 实验2-1 点亮LED
 ┃ ┃ ┣ BUILD.gn
 ┃ ┃ ┗ led_demo.c
 ┃ ┣ 实验2-2 按键控制LED
 ┃ ┃ ┣ BUILD.gn
 ┃ ┃ ┗ led_set_demo.c
 ┃ ┣ 实验3 使用按键控制LED亮灭
 ┃ ┃ ┣ BUILD.gn
 ┃ ┃ ┗ adc_led.c
 ┃ ┣ 实验4 OLED屏幕显示学号姓名
 ┃ ┃ ┗ ...
 ┃ ┣ 实验5-1 PWM实现LED呼吸灯
 ┃ ┃ ┗ ...
 ┃ ┣ 实验5-2 开发板连接WiFi热点并在OLED显示
 ┃ ┃ ┗ ...
 ┃ ┣ 实验6 开发板和电脑通过MQTT代理互发消息
 ┃ ┃ ┗ ...
 ┃ ┗ 智能小车
 ┃ ┃ ┣ ssd1306						# OLED显示依赖
 ┃ ┃ ┃ ┣ BUILD.gn
 ┃ ┃ ┃ ┣ ssd1306.c
 ┃ ┃ ┃ ┣ ssd1306.h
 ┃ ┃ ┃ ┣ ssd1306_conf.h
 ┃ ┃ ┃ ┣ ssd1306_fonts.c
 ┃ ┃ ┃ ┗ ssd1306_fonts.h
 ┃ ┃ ┣ 前端								 # 前端Wi-Fi连接、控制小车插件
 ┃ ┃ ┃ ┗ 前端
 ┃ ┃ ┃ ┃ ┗ windows_car_controller.py
 ┃ ┃ ┣ BUILD.gn
 ┃ ┃ ┣ README.md					# 代码具体说明
 ┃ ┃ ┣ car_tcp_server_test.c
 ┃ ┃ ┣ net_common.h
 ┃ ┃ ┣ net_demo.h
 ┃ ┃ ┣ net_params.h
 ┃ ┃ ┣ robot_control.c
 ┃ ┃ ┣ robot_control.h
 ┃ ┃ ┣ robot_hcsr04.c
 ┃ ┃ ┣ robot_l9110s.c
 ┃ ┃ ┣ robot_sg90.c
 ┃ ┃ ┣ ssd1306_test.c
 ┃ ┃ ┣ trace_model.c
 ┃ ┃ ┣ wifi_connecter.c
 ┃ ┃ ┗ wifi_connecter.h
 ┣ .gitignore
 ┣ README.md
 ┗ 电子工艺实验报告.pdf
```



## 基于OpenHarmony的智能循迹与避障小车

​	本项目是厦门大学2023级计算机科学与技术专业课程《电子工艺实训》的个人项目，基于OpenHarmony (LiteOS) 实时操作系统开发。项目包含课程前几项实验代码以及智能小车部分代码。智能小车的主要功能为循迹（黑线），避障，并支持通过Wi-Fi进行远程遥控。

​	课程的前几个基础实验是小车项目的理论基石，请学习参透后再开始小车项目。以下仅对小车内容进行介绍。

### 核心功能

- 基于RTOS的多线程架构：系统的核心功能，如超声波测距、按键扫描和主控制逻辑，均在独立的任务(Task)中运行，确保了系统的实时响应性，避免了逻辑阻塞。
- 循迹算法：循迹的主逻辑为一套包含7种状态的有限状态机，小车能够精确处理直行、转弯（包含小角度弯、直角弯、以及大角度急弯）、十字路口及终点等多种复杂赛道情况。
- 智能避障：基于“感知-思考-行动”模型，小车通过舵机控制超声波传感器主动扫描周围环境，并运用贪心算法来自主决策最优的避障路径。
- Wi-Fi远程控制系统：
  - 小车端：使用C语言实现了一个TCP服务器，并自定义一套通信协议用于解析控制指令。
  - PC端：使用Python和Tkinter库开发了一个图形化控制器，用于向小车发送指令。

### 系统架构

​	系统采用分层、模块化的设计思想。主状态机负责在`循迹`、`避障`、`远程控制`三种模式间切换。在循迹模式内部，一个更复杂的7状态状态机负责具体的导航逻辑。所有硬件的交互都通过GPIO, PWM, ADC, I2C等驱动接口进行封装调用。

​	更详细的系统设计、算法流程与实现细节，请参阅完整的**[项目报告](https://github.com/Yowe05m/OpenHarmony-car/blob/main/%E7%94%B5%E5%AD%90%E5%B7%A5%E8%89%BA%E5%AE%9E%E9%AA%8C%E6%8A%A5%E5%91%8A.pdf)**。

## 构建与运行

### 固件 (小车端)

1. **环境要求**：
   - 华为 DevEco Device Tool 一站式集成开发环境
   - DevTools_Hi3861V100_v1.0 开发工具
   - 3.0-lts 工程文件
2. **烧录步骤**：
   - 将实验代码放置在 ./applications/sample/wifi-iot/app/ 下
   - 编写模块BUILD.gn文件，指定需参与构建的特性模块
   - 编译项目，并将生成的固件烧录到Hi3861开发板

### PC控制器 (上位机)

1. **环境要求**:
   - Python 3.x
2. **运行步骤**:
   - 确保小车已开机，并成功连接到与PC相同的Wi-Fi网络（连接成功后，小车的IP地址会显示在终端中。为了监控此IP地址，请务必将小车与PC机连接，并处于Monitor状态）。
   - 运行控制器脚本: `python windows_car_controller.py`
   - 在界面中输入小车的IP地址，点击“连接”即可开始遥控。