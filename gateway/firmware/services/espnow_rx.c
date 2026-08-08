#include "espnow_rx.h"
#include "app_config.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "string.h"
#include "esp_log.h"
typedef struct { size_t len; int rssi; char data[APP_JSON_MAX+1]; } rx_frame_t;
static QueueHandle_t q; static const char *TAG="espnow";
static void recv_cb(const esp_now_recv_info_t *info,const uint8_t *data,int len){if(!data||len<=0||len>APP_JSON_MAX)return;rx_frame_t f={.len=(size_t)len,.rssi=(info&&info->rx_ctrl)?info->rx_ctrl->rssi:-127};memcpy(f.data,data,(size_t)len);f.data[len]=0;xQueueSend(q,&f,0);}
void espnow_rx_start(void){q=xQueueCreate(APP_RX_QUEUE_LENGTH,sizeof(rx_frame_t));ESP_ERROR_CHECK(esp_now_init());ESP_ERROR_CHECK(esp_now_register_recv_cb(recv_cb));ESP_LOGI(TAG,"ESP-NOW ready; channel follows WiFi");}
bool espnow_rx_receive(char *data,size_t *len,int *rssi,size_t capacity){if(!q||!data||!len||!rssi||capacity<2)return false;rx_frame_t f;if(xQueueReceive(q,&f,pdMS_TO_TICKS(100))!=pdTRUE)return false;if(f.len+1>capacity)return false;memcpy(data,f.data,f.len+1);*len=f.len;*rssi=f.rssi;return true;}
