
#ifndef NET_DEMO_COMMON_H
#define NET_DEMO_COMMON_H

#include <stdio.h>

void NetDemoTest(unsigned short port, const char* host);

const char* GetNetDemoName(void);

#define IMPL_GET_NET_DEMO_NAME(testFunc) \
    const char* GetNetDemoName() { \
        static const char* demoName = #testFunc; \
        return demoName; \
    }

#define SERVER_TEST_DEMO(testFunc) \
    void NetDemoTest(unsigned short port, const char* host) { \
        (void) host; \
        printf("______________________________________________________________________________\r\n"); \
        printf("%s start\r\n", #testFunc); \
        printf("_______________________________________\r\n"); \
        printf("TCP server port is :%hu \r\n", port); \
        testFunc(port); \
    } \
    IMPL_GET_NET_DEMO_NAME(testFunc)

#endif // NET_DEMO_COMMON_H