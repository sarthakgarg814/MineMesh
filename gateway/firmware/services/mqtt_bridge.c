#include "mqtt_bridge.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include "led_status.h"
static esp_mqtt_client_handle_t client; static bool connected; static const char *topic; static const char *TAG="mqtt";
static void event(void*a,esp_event_base_t b,int32_t id,void*d){if(id==MQTT_EVENT_CONNECTED){connected=true;ESP_LOGI(TAG,"MQTT connected");}else if(id==MQTT_EVENT_DISCONNECTED){connected=false;led_status_error();ESP_LOGW(TAG,"MQTT disconnected");}else if(id==MQTT_EVENT_PUBLISHED)ESP_LOGI(TAG,"publish success");}
bool mqtt_bridge_connected(void){return connected;}
void mqtt_bridge_start(const char *uri,const char *t){topic=t;esp_mqtt_client_config_t cfg={.broker.address.uri=uri};client=esp_mqtt_client_init(&cfg);esp_mqtt_client_register_event(client,ESP_EVENT_ANY_ID,event,NULL);esp_mqtt_client_start(client);}
bool mqtt_bridge_publish(const char *payload,int len){if(!connected)return false;int id=esp_mqtt_client_publish(client,topic,payload,len,1,0);if(id<0){ESP_LOGE(TAG,"publish failed");return false;}return true;}
