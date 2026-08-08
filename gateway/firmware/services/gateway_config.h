#ifndef GATEWAY_CONFIG_H
#define GATEWAY_CONFIG_H
#include "esp_err.h"
#include <stdbool.h>
#define GATEWAY_TEXT_MAX 128
typedef enum { GATEWAY_MODE_AP_ESPNOW = 0, GATEWAY_MODE_WIFI_MQTT = 1 } gateway_mode_t;
typedef struct { gateway_mode_t mode; char wifi_ssid[GATEWAY_TEXT_MAX]; char wifi_password[GATEWAY_TEXT_MAX]; char mqtt_broker[GATEWAY_TEXT_MAX]; char mqtt_topic_uplink[GATEWAY_TEXT_MAX]; char gateway_id[32]; } gateway_config_t;
esp_err_t gateway_config_load(gateway_config_t *config);
esp_err_t gateway_config_save(const gateway_config_t *config);
esp_err_t gateway_config_factory_reset(void);
bool gateway_config_valid(const gateway_config_t *config);
#endif
