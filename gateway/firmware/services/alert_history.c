#include "alert_history.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "cJSON.h"
#include <string.h>
#include <stdlib.h>
#define ALERT_MAX 10
#define ALERT_RAW_MAX 250
typedef struct { char message_id[80]; char source_id[40]; char type[20]; char reason[32]; int battery; int rssi; int64_t timestamp; uint32_t age_ms; bool active; } alert_t;
static alert_t alerts[ALERT_MAX]; static size_t count; static SemaphoreHandle_t lock;
static cJSON *get(cJSON *o,const char *k){return cJSON_GetObjectItemCaseSensitive(o,k);}
static int number(cJSON *o,const char *k,int fallback){cJSON *v=get(o,k);return cJSON_IsNumber(v)?v->valueint:fallback;}
void alert_history_init(void){if(!lock)lock=xSemaphoreCreateMutex();if(lock)xSemaphoreTake(lock,portMAX_DELAY);memset(alerts,0,sizeof(alerts));count=0;if(lock)xSemaphoreGive(lock);}
void alert_history_add(const char *raw,size_t len,const protocol_meta_t *meta){if(!raw||!meta||len==0||len>ALERT_RAW_MAX||!lock)return;cJSON *o=cJSON_ParseWithLength(raw,len);if(!o)return;cJSON *p=get(o,"payload");alert_t a={0};strlcpy(a.message_id,meta->message_id,sizeof(a.message_id));strlcpy(a.source_id,meta->source_id,sizeof(a.source_id));strlcpy(a.type,meta->message_type,sizeof(a.type));strlcpy(a.reason,meta->critical_reason,sizeof(a.reason));a.battery=number(p,"battery",-1);a.rssi=number(p,"rssi",-127);a.timestamp=number(o,"timestamp",0);a.age_ms=0;a.active=true;cJSON_Delete(o);if(xSemaphoreTake(lock,pdMS_TO_TICKS(20))!=pdTRUE)return;if(count==ALERT_MAX){memmove(&alerts[1],&alerts[0],sizeof(alerts[0])*(ALERT_MAX-1));count=ALERT_MAX-1;}else if(count>0){memmove(&alerts[1],&alerts[0],sizeof(alerts[0])*count);}alerts[0]=a;count++;for(size_t i=1;i<count;i++)alerts[i].active=false;xSemaphoreGive(lock);}
size_t alert_history_json(char *out,size_t capacity){if(!out||capacity<3)return 0;cJSON *root=cJSON_CreateObject();cJSON *list=cJSON_AddArrayToObject(root,"alerts");int active=0;if(lock)xSemaphoreTake(lock,pdMS_TO_TICKS(50));for(size_t i=0;i<count;i++){cJSON *a=cJSON_CreateObject();cJSON_AddStringToObject(a,"message_id",alerts[i].message_id);cJSON_AddStringToObject(a,"source_id",alerts[i].source_id);cJSON_AddStringToObject(a,"type",alerts[i].type);cJSON_AddStringToObject(a,"reason",alerts[i].reason);cJSON_AddNumberToObject(a,"battery",alerts[i].battery);cJSON_AddNumberToObject(a,"rssi",alerts[i].rssi);cJSON_AddNumberToObject(a,"timestamp",alerts[i].timestamp);cJSON_AddBoolToObject(a,"active",alerts[i].active);if(alerts[i].active)active++;cJSON_AddItemToArray(list,a);}if(lock)xSemaphoreGive(lock);cJSON_AddBoolToObject(root,"active",active>0);char *s=cJSON_PrintUnformatted(root);size_t n=s?strlen(s):0;if(n>=capacity)n=capacity-1;if(s)memcpy(out,s,n);out[n]='\0';free(s);cJSON_Delete(root);return n;}
void alert_history_acknowledge(void){if(!lock)return;if(xSemaphoreTake(lock,pdMS_TO_TICKS(50))==pdTRUE){for(size_t i=0;i<count;i++)alerts[i].active=false;xSemaphoreGive(lock);}}
bool alert_history_active(void){bool active=false;if(lock&&xSemaphoreTake(lock,pdMS_TO_TICKS(20))==pdTRUE){active=count>0&&alerts[0].active;xSemaphoreGive(lock);}return active;}
