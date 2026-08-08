#ifndef WIFI_SERVICE_H
#define WIFI_SERVICE_H
#include <stdbool.h>
#include <stdint.h>
void wifi_service_start(const char *ssid,const char *password);
void wifi_service_start_ap(void);
bool wifi_service_connected(void);
bool wifi_service_wait_connected(uint32_t timeout_ms);
bool wifi_service_get_sta_mac(uint8_t mac[6]);
uint8_t wifi_service_get_channel(void);
bool wifi_service_get_ap_mac(uint8_t mac[6]);
#endif
