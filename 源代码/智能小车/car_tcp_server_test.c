#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h> // 需要包含此头文件以使用atoi
#include "robot_control.h"
#include "net_demo.h"
#include "net_common.h"

// 外部函数声明
extern void robot_init(void); // 新增初始化函数
extern void car_backward(unsigned int speed);
extern void car_forward(unsigned int speed);
extern void car_left(unsigned int speed);
extern void car_right(unsigned int speed);
extern void car_stop(void);

#define DELAY_1S  (1)

void TcpServerTest(unsigned short port)
{
    ssize_t retval = 0;
    int backlog = 1;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    int connfd = -1;
    struct sockaddr_in clientAddr = {0};
    socklen_t clientAddrLen = sizeof(clientAddr);
    struct sockaddr_in serverAddr = {0};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    retval = bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)); 
    if (retval < 0) {
        printf("bind failed, %ld!\r\n", retval);
        return;       
    }
    printf("bind to port %hu success!\r\n", port);

    retval = listen(sockfd, backlog);
    if (retval < 0) {
        printf("listen failed!\r\n");
        return;
    }
    printf("listen with %d backlog success!\r\n", backlog);
    
    // 在启动服务器前初始化机器人电机 ===
    robot_init();
    car_stop(); // 确保启动时电机是停止的
    // ===========================================

   while (g_car_status == CAR_REMOTE_CONTROL_STATUS) {
        connfd = accept(sockfd, (struct sockaddr *)&clientAddr, &clientAddrLen);
        if (g_car_status != CAR_REMOTE_CONTROL_STATUS) {
        break;
    }
        if (connfd < 0) {
            printf("accept failed, %d, %d\r\n", connfd, errno);
            continue;
        }
        printf("accept success, connfd = %d!\r\n", connfd);
        printf("client addr info: host = %s, port = %hu\r\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));

       while (g_car_status == CAR_REMOTE_CONTROL_STATUS) {
           char request[128] = "";
           retval = recv(connfd, request, sizeof(request) - 1, 0); // 留一个字节给'\0'
           if (retval <= 0) { // 如果返回值是0或负数，都表示连接已断开
               printf("recv failed or client disconnected, retval = %ld\r\n", retval);
               car_stop(); // 客户端断开连接，小车停止
               close(connfd);
               break;
           }
           request[retval] = '\0'; // 确保字符串正确结尾
           printf("The data received from the client is '%s'\r\n", request);

           // === 修改：解析新的指令格式 ===
           char command = request[0];
           unsigned int speed = 0;

           if (strlen(request) > 1) {
               speed = atoi(&request[1]); // 从第二个字符开始转换为整数
           }

           switch (command) {
              case 'F': // 前进
                  printf("car_forward with speed %u\n", speed);
                  car_forward(speed);
                  break;  
              case 'B': // 后退
                  printf("car_backward with speed %u\n", speed);
                  car_backward(speed);
                  break;  
              case 'L': // 左转
                  printf("car_left with speed %u\n", speed);
                  car_left(speed);
                  break;    
              case 'R': // 右转
                  printf("car_right with speed %u\n", speed);
                  car_right(speed);
                  break;
              case 'S': // 停止
              default:
                  printf("car_stop\n");
                  car_stop();
                  break;   
           }
           // ===============================

           // 回复客户端，确认收到
           retval = send(connfd, request, strlen(request), 0);
           if (retval <= 0) {
               printf("send response failed, retval = %ld\r\n", retval);
               car_stop(); // 发送失败，可能连接已断开
               close(connfd);
               break;
           }
        }
    if (connfd >= 0) {
        close(connfd);
        connfd = -1;
    }
    }
    close(sockfd); 
}

SERVER_TEST_DEMO(TcpServerTest);