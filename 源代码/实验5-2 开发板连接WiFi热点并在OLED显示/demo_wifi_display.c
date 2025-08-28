/*
 * Copyright (c) 2020 HiSilicon (Shanghai) Technologies CO., LIMITED.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>    // malloc, free
#include <string.h>    // strncpy
#include <securec.h>   // memcpy_s, errno_t
#include <stdio.h>

#include <unistd.h>
#include "hi_wifi_api.h"
#include "wifi_sta.h"
#include "lwip/ip_addr.h"
#include "lwip/netifapi.h"

#include "ohos_init.h"         // SYS_RUN 宏
#include "cmsis_os2.h"         // 线程接口

#include "ssd1306_oled.h"
#include "hi_io.h"            // IO 复用


#define APP_INIT_VAP_NUM    1
#define APP_INIT_USR_NUM    1

#define WIFI_SSID       "Ma"           // ← 把这里改成你的 SSID
#define WIFI_PASSWORD   "mamamama"       // ← 把这里改成你的密码
static const char *target_ssid     = WIFI_SSID;
static const char *target_password = WIFI_PASSWORD;
static int ssid_len  = sizeof(WIFI_SSID) - 1;
static int pwd_len   = sizeof(WIFI_PASSWORD) - 1;

// 存储要显示的 SSID
#define MAX_SSID_LEN 32
static char g_connected_ssid[MAX_SSID_LEN + 1] = {0};

static struct netif *g_lwip_netif = NULL;

/* 重置 IP 地址, gateway and netmask */
void hi_sta_reset_addr(struct netif *pst_lwip_netif)
{
    ip4_addr_t st_gw;
    ip4_addr_t st_ipaddr;
    ip4_addr_t st_netmask;

    if (pst_lwip_netif == NULL) {
        printf("hisi_reset_addr::Null param of netdev\r\n");
        return;
    }

    IP4_ADDR(&st_gw, 0, 0, 0, 0);
    IP4_ADDR(&st_ipaddr, 0, 0, 0, 0);
    IP4_ADDR(&st_netmask, 0, 0, 0, 0);

    netifapi_netif_set_addr(pst_lwip_netif, &st_ipaddr, &st_netmask, &st_gw);
}

void wifi_wpa_event_cb(const hi_wifi_event *hisi_event)
{
    if (hisi_event == NULL)
        return;

    switch (hisi_event->event) {
        case HI_WIFI_EVT_SCAN_DONE:
            printf("WiFi: Scan results available\n");
            break;
        case HI_WIFI_EVT_CONNECTED:
            printf("WiFi: Connected\n");
            netifapi_dhcp_start(g_lwip_netif);
            // OLED 上显示已连接及 SSID
            Oled_Init();
            Oled_ShowString(0, 0, "WiFi: Connected");
            Oled_ShowString(0, 16, g_connected_ssid);
            break;
        case HI_WIFI_EVT_DISCONNECTED:
            printf("WiFi: Disconnected\n");
            netifapi_dhcp_stop(g_lwip_netif);
            hi_sta_reset_addr(g_lwip_netif);
            // OLED 上显示断开
            Oled_ShowString(0, 0, "WiFi: Disconnected");
            break;
        case HI_WIFI_EVT_WPS_TIMEOUT:
            printf("WiFi: wps is timeout\n");
            break;
        default:
            break;
    }
}

int hi_wifi_start_connect(void)
{
    int ret;
    errno_t rc;
    hi_wifi_assoc_request assoc_req = {0};

    // 拷贝 SSID
    rc = memcpy_s(assoc_req.ssid, sizeof(assoc_req.ssid), target_ssid, ssid_len);
    if (rc != EOK) { 
        return -1; 
    }

    // 记录下来，供 OLED 显示
    strncpy(g_connected_ssid, target_ssid, MAX_SSID_LEN);

    assoc_req.auth = HI_WIFI_SECURITY_WPA2PSK;
    rc = memcpy_s(assoc_req.key, sizeof(assoc_req.key), target_password, pwd_len);
    if (rc != EOK) { 
        return -1; 
    }

    ret = hi_wifi_sta_connect(&assoc_req);
    if (ret != HISI_OK) {
        return -1;
    }

    return 0;
}

int hi_wifi_start_sta(void)
{
    int ret;
    char ifname[WIFI_IFNAME_MAX_SIZE + 1] = {0};
    int len = sizeof(ifname);

    const unsigned char wifi_vap_res_num = APP_INIT_VAP_NUM;
    const unsigned char wifi_user_res_num = APP_INIT_USR_NUM;
    unsigned int  num = WIFI_SCAN_AP_LIMIT;

    // 1) 初始化 Wi-Fi
    ret = hi_wifi_init(wifi_vap_res_num, wifi_user_res_num);
    printf("DBG: hi_wifi_init( %d, %d ) => %d\n",
       APP_INIT_VAP_NUM, APP_INIT_USR_NUM, ret);    
    if (ret < 0) {
        return -1;
    }

    // 2) 启动 STA 模式
    ret = hi_wifi_sta_start(ifname, &len);
    printf("DBG: hi_wifi_sta_start() => %d, ifname=\"%s\", len=%d\n", ret, ifname, len);
    if (ret < 0) {
        return -1;
    }

    // 3) 注册事件回调
    ret = hi_wifi_register_event_callback(wifi_wpa_event_cb);
    printf("DBG: hi_wifi_register_event_callback() => %d\n", ret);
    if (ret < 0) {
        printf("register wifi event callback failed\n");
    }

    // 4) 查找 netif
    g_lwip_netif = netifapi_netif_find(ifname);
    printf("DBG: netifapi_netif_find(\"%s\") => %p\n", ifname, (void *)g_lwip_netif);
    if (g_lwip_netif == NULL) {
        printf("%s: get netif failed\n", __FUNCTION__);
        return -1;
    }

    // 5) 扫描
    ret = hi_wifi_sta_scan();
    printf("DBG: hi_wifi_sta_scan() => %d\n", ret);
    if (ret < 0) {
        return -1;
    }

    sleep(5);   /* sleep 5s, waiting for scan result. */

    hi_wifi_ap_info *pst_results = malloc(sizeof(hi_wifi_ap_info) * WIFI_SCAN_AP_LIMIT);
    if (pst_results == NULL) {
        return -1;
    }

    ret = hi_wifi_sta_scan_results(pst_results, &num);
    if (ret < 0) {
        free(pst_results);
        return -1;
    }

    for (unsigned int loop = 0; (loop < num) && (loop < WIFI_SCAN_AP_LIMIT); loop++) {
        printf("SSID: %s\n", pst_results[loop].ssid);
    }
    free(pst_results);

    /* if received scan results, select one SSID to connect */
    ret = hi_wifi_start_connect();
    if (ret < 0) {
        return -1;
    }

    return 0;
}

void hi_wifi_stop_sta(void)
{
    int ret;

    ret = hi_wifi_sta_stop();
    if (ret != HISI_OK) {
        printf("failed to stop sta\n");
    }

    ret = hi_wifi_deinit();
    if (ret != HISI_OK) {
        printf("failed to deinit wifi\n");
    }

    g_lwip_netif = NULL;
}

/* —— 2. 任务函数：初始化 OLED + 启动 Wi-Fi —— */
static void WifiStaTask(void *arg)
{
    (void)arg;

    // 1) 初始化 OLED 屏幕
    Oled_Init();

    // 2) 启动 Wi-Fi
    if (hi_wifi_start_sta() != 0) {
        printf("WiFi start failed\n");
        Oled_ShowString(0, 0, "WiFi Start Fail");
        return;
    }

    // 立刻显示“Connecting…”
    Oled_ShowString(0, 0, "WiFi: Connecting");
}

/* —— 3. 入口：系统启动后创建线程 —— */
static void WifiStaEntry(void)
{
    osThreadAttr_t attr = {
        .name       = "WifiStaTask",
        .stack_size = 4096,
        .priority   = osPriorityNormal,
    };
    if (osThreadNew(WifiStaTask, NULL, &attr) == NULL) {
        printf("Failed to create WifiStaTask!\n");
    }
}

SYS_RUN(WifiStaEntry);