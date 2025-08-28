#ifndef NET_PARAMS_H
#define NET_PARAMS_H

#ifndef PARAM_HOTSPOT_SSID
#define PARAM_HOTSPOT_SSID "Ma"        // your AP SSID
#endif

#ifndef PARAM_HOTSPOT_PSK
#define PARAM_HOTSPOT_PSK  "md205525"  // your AP PSK
#endif

#ifndef PARAM_HOTSPOT_TYPE
#define PARAM_HOTSPOT_TYPE WIFI_SEC_TYPE_PSK // defined in wifi_device_config.h
#endif

#ifndef PARAM_SERVER_ADDR
#define PARAM_SERVER_ADDR "0.0.0.0" // 可以接入任意的ip地址的TCP服务端
#endif

#ifndef PARAM_SERVER_PORT
#define PARAM_SERVER_PORT 5678              // 3861开发板TCP socket端口号
#endif

#endif  // NET_PARAMS_H