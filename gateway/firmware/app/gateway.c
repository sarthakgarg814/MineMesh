#include "gateway.h"
#include "gateway_config.h"
#include "wifi_service.h"
#include "mqtt_bridge.h"
#include "espnow_rx.h"
#include "protocol_v1.h"
#include "dedup.h"
#include "cli.h"
#include "provisioning_web.h"
#include "worker_status.h"
#include "alert_history.h"
#include "led_status.h"
#include "boot_button.h"
#include "app_config.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "string.h"
static const char *TAG="gateway";
static void reset_gateway_config(void){esp_err_t e=gateway_config_factory_reset();if(e==ESP_OK)esp_restart();ESP_LOGE(TAG,"configuration reset failed: %s",esp_err_to_name(e));}
static gateway_config_t active_config;
static void process_task(void *arg){(void)arg;char raw[APP_JSON_MAX+1];size_t len;int rssi;protocol_meta_t meta;for(;;){if(active_config.mode==GATEWAY_MODE_WIFI_MQTT){char retry[APP_JSON_MAX+1];size_t retry_len;while(mqtt_bridge_connected()&&retry_dequeue(retry,&retry_len,sizeof(retry)))mqtt_bridge_publish(retry,(int)retry_len);}if(!espnow_rx_receive(raw,&len,&rssi,sizeof(raw)))continue;gateway_cli_set_last_rx(raw,len);if(!protocol_v1_validate(raw,len,active_config.gateway_id,&meta)){led_status_error();ESP_LOGW(TAG,"invalid Protocol v1 packet");continue;}worker_status_update(meta.source_id,meta.message_type,meta.priority,rssi);led_status_rx();ESP_LOGI(TAG,"valid Protocol v1 RX source=%s type=%s priority=%s rssi=%d",meta.source_id,meta.message_type,meta.priority,rssi);if(dedup_seen_or_add(meta.message_id))continue;if(!strcmp(meta.message_type,"SOS")||!strcmp(meta.message_type,"FALL_ALERT")){alert_history_add(raw,len,&meta);if(!strcmp(meta.message_type,"FALL_ALERT"))led_status_fall();else led_status_sos();}if(active_config.mode==GATEWAY_MODE_AP_ESPNOW)continue;if(mqtt_bridge_publish(raw,(int)len)){gateway_cli_set_last_publish(raw,len);led_status_tx();}else if(!strcmp(meta.priority,"CRITICAL"))retry_enqueue(raw,len);}}
void gateway_start(void){led_status_init();worker_status_init();alert_history_init();esp_err_t e=nvs_flash_init();if(e==ESP_ERR_NVS_NO_FREE_PAGES||e==ESP_ERR_NVS_NEW_VERSION_FOUND){ESP_ERROR_CHECK(nvs_flash_erase());ESP_ERROR_CHECK(nvs_flash_init());}boot_button_start(reset_gateway_config);ESP_ERROR_CHECK(gateway_config_load(&active_config));if(active_config.mode==GATEWAY_MODE_AP_ESPNOW){led_status_ap();wifi_service_start_ap();uint8_t mac[6];if(wifi_service_get_ap_mac(mac))ESP_LOGI(TAG,"Gateway AP MAC: %02X:%02X:%02X:%02X:%02X:%02X channel=%u",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],wifi_service_get_channel());espnow_rx_start();ESP_ERROR_CHECK(provisioning_web_start());}else if(gateway_config_valid(&active_config)){led_status_wifi();wifi_service_start(active_config.wifi_ssid,active_config.wifi_password);espnow_rx_start();if(wifi_service_wait_connected(30000)){mqtt_bridge_start(active_config.mqtt_broker,active_config.mqtt_topic_uplink);led_status_mqtt();}ESP_ERROR_CHECK(provisioning_web_start());}else{active_config.mode=GATEWAY_MODE_AP_ESPNOW;led_status_ap();wifi_service_start_ap();espnow_rx_start();ESP_ERROR_CHECK(provisioning_web_start());}xTaskCreate(process_task,"gateway_process",4096,NULL,5,NULL);gateway_cli_start();ESP_LOGI(TAG,"gateway ready");}
