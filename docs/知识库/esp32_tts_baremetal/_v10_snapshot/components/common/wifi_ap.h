#ifndef __WIFI_AP_H
#define __WIFI_AP_H

/* 启动 SoftAP 热点：SSID=EV-Car-Setup（开放网络），IP=192.168.4.1
 * 要求：调用前已完成 nvs_flash_init（params_init 内已做） */
void wifi_ap_start(void);

#endif /* __WIFI_AP_H */
