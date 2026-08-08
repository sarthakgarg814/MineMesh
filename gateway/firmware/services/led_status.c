#include "led_status.h"
#include "app_config.h"
#include "led_strip.h"
#include "led_strip_rmt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
static led_strip_handle_t strip; static uint32_t base_r,base_g,base_b; static volatile uint8_t pulse; static volatile uint8_t alert_pattern; static const char *TAG="led";
static void set(uint32_t r,uint32_t g,uint32_t b){if(!strip)return;led_strip_set_pixel(strip,0,r,g,b);led_strip_refresh(strip);}
static void task(void *arg){(void)arg;for(;;){if(alert_pattern){uint8_t p=alert_pattern;uint32_t r=p==1?220:220,g=0,b=p==1?0:100;set(r,g,b);vTaskDelay(pdMS_TO_TICKS(180));set(0,0,0);vTaskDelay(pdMS_TO_TICKS(120));set(r,g,b);vTaskDelay(pdMS_TO_TICKS(180));set(0,0,0);if(p==1){vTaskDelay(pdMS_TO_TICKS(120));set(r,g,b);vTaskDelay(pdMS_TO_TICKS(180));set(0,0,0);}set(base_r,base_g,base_b);}else if(pulse){uint8_t p=pulse;pulse=0;set(0,180,180);vTaskDelay(pdMS_TO_TICKS(90));set(0,0,0);vTaskDelay(pdMS_TO_TICKS(90));if(p>1){set(0,180,180);vTaskDelay(pdMS_TO_TICKS(90));set(0,0,0);}set(base_r,base_g,base_b);}vTaskDelay(pdMS_TO_TICKS(60));}}
void led_status_init(void){led_strip_config_t cfg={.strip_gpio_num=APP_STATUS_LED_GPIO,.max_leds=1,.led_model=LED_MODEL_WS2812,.color_component_format=LED_STRIP_COLOR_COMPONENT_FMT_GRB};led_strip_rmt_config_t rmt={.resolution_hz=10*1000*1000};if(led_strip_new_rmt_device(&cfg,&rmt,&strip)!=ESP_OK){ESP_LOGW(TAG,"RGB LED unavailable");return;}led_strip_clear(strip);ESP_LOGI(TAG,"RGB LED initialized on GPIO%d",APP_STATUS_LED_GPIO);xTaskCreate(task,"led_status",4096,NULL,2,NULL);}
static void base(uint32_t r,uint32_t g,uint32_t b){base_r=r;base_g=g;base_b=b;set(r,g,b);}void led_status_ap(void){base(0,0,100);}void led_status_wifi(void){base(120,60,0);}void led_status_mqtt(void){base(0,100,0);}void led_status_rx(void){pulse=2;}void led_status_tx(void){pulse=2;}void led_status_error(void){pulse=1;}void led_status_critical(void){pulse=3;}void led_status_sos(void){alert_pattern=2;}void led_status_fall(void){alert_pattern=1;}void led_status_acknowledge(void){alert_pattern=0;set(base_r,base_g,base_b);}
