#include "wifi_service.h"
#include "app_config.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
static volatile bool connected; static bool wifi_initialized; static bool handlers_registered; static esp_netif_t *sta_netif; static esp_netif_t *ap_netif; static const char *TAG="wifi";
static void handler(void*a,esp_event_base_t b,int32_t id,void*d){if(b==WIFI_EVENT&&id==WIFI_EVENT_STA_DISCONNECTED){connected=false;esp_wifi_connect();ESP_LOGW(TAG,"WiFi disconnected; retrying");}if(b==IP_EVENT&&id==IP_EVENT_STA_GOT_IP){connected=true;ESP_LOGI(TAG,"WiFi connected");}}
bool wifi_service_connected(void){return connected;}
bool wifi_service_wait_connected(uint32_t timeout_ms){uint32_t waited=0;while(!connected&&waited<timeout_ms){vTaskDelay(pdMS_TO_TICKS(250));waited+=250;}return connected;}
bool wifi_service_get_sta_mac(uint8_t mac[6]){return mac&&esp_wifi_get_mac(WIFI_IF_STA,mac)==ESP_OK;}
uint8_t wifi_service_get_channel(void){uint8_t primary=0;wifi_second_chan_t second=WIFI_SECOND_CHAN_NONE;return esp_wifi_get_channel(&primary,&second)==ESP_OK?primary:0;}
bool wifi_service_get_ap_mac(uint8_t mac[6]){return mac&&esp_wifi_get_mac(WIFI_IF_AP,mac)==ESP_OK;}
static void wifi_service_init_once(void){if(!wifi_initialized){ESP_ERROR_CHECK(esp_netif_init());esp_err_t e=esp_event_loop_create_default();if(e!=ESP_OK&&e!=ESP_ERR_INVALID_STATE)ESP_ERROR_CHECK(e);wifi_init_config_t cfg=WIFI_INIT_CONFIG_DEFAULT();ESP_ERROR_CHECK(esp_wifi_init(&cfg));wifi_initialized=true;}if(!handlers_registered){ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT,ESP_EVENT_ANY_ID,handler,NULL));ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT,IP_EVENT_STA_GOT_IP,handler,NULL));handlers_registered=true;}}
void wifi_service_start(const char *ssid,const char *password){wifi_service_init_once();if(!sta_netif)sta_netif=esp_netif_create_default_wifi_sta();wifi_config_t c={0};strlcpy((char*)c.sta.ssid,ssid,sizeof(c.sta.ssid));strlcpy((char*)c.sta.password,password,sizeof(c.sta.password));ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA,&c));ESP_ERROR_CHECK(esp_wifi_start());}
void wifi_service_start_ap(void){wifi_service_init_once();if(!ap_netif)ap_netif=esp_netif_create_default_wifi_ap();wifi_config_t c={0};strlcpy((char*)c.ap.ssid,"MineMesh-Gateway",sizeof(c.ap.ssid));strlcpy((char*)c.ap.password,"minemesh",sizeof(c.ap.password));c.ap.ssid_len=strlen((char*)c.ap.ssid);c.ap.authmode=WIFI_AUTH_WPA2_PSK;c.ap.max_connection=4;c.ap.channel=APP_ESPNOW_TEST_CHANNEL;ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP,&c));ESP_ERROR_CHECK(esp_wifi_start());ESP_LOGI(TAG,"setup AP MineMesh-Gateway password=minemesh");}
