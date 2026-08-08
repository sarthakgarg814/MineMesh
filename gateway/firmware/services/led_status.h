#ifndef LED_STATUS_H
#define LED_STATUS_H
void led_status_init(void);
void led_status_ap(void);
void led_status_wifi(void);
void led_status_mqtt(void);
void led_status_rx(void);
void led_status_tx(void);
void led_status_error(void);
void led_status_critical(void);
void led_status_sos(void);
void led_status_fall(void);
void led_status_acknowledge(void);
#endif
