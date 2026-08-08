#include "app.h"
#include "app_config.h"
#include "logger.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "led_strip.h"
#include "led_strip_rmt.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <inttypes.h>
#include <time.h>

static const char *TAG = "minemesh_worker";
static i2c_master_dev_handle_t imu;
static led_strip_handle_t led;
static adc_oneshot_unit_handle_t adc;
static bool imu_ok;
static bool gateway_seen;
static uint8_t gateway_mac[6];
static int last_rssi = -127;
static uint32_t sequence;
static int64_t last_motion_ms;
static bool inactivity_sent;
static bool fall_pending;
static bool fall_alert_episode;
static int64_t fall_started_ms;
static bool sos_active;
static const char *last_tx_type = "UNKNOWN";

static void on_espnow_send(const esp_now_send_info_t *tx_info, esp_now_send_status_t status)
{
    (void)tx_info;
    ESP_LOGI(TAG, "TX_RESULT type=%s status=%s", last_tx_type,
             status == ESP_NOW_SEND_SUCCESS ? "SENT" : "FAILED");
}

static esp_err_t imu_read(uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(imu, &reg, 1, data, len, pdMS_TO_TICKS(100));
}

static bool imu_sample(float *ax, float *ay, float *az, float *temp)
{
    uint8_t b[14];
    if (!imu_ok || imu_read(0x3B, b, sizeof(b)) != ESP_OK) return false;
    int16_t x = (int16_t)((b[0] << 8) | b[1]);
    int16_t y = (int16_t)((b[2] << 8) | b[3]);
    int16_t z = (int16_t)((b[4] << 8) | b[5]);
    int16_t t = (int16_t)((b[6] << 8) | b[7]);
    *ax = x / 16384.0f; *ay = y / 16384.0f; *az = z / 16384.0f;
    *temp = t / 340.0f + 36.53f;
    return true;
}

static int battery_percent(void)
{
    int raw = 0;
    if (adc && adc_oneshot_read(adc, ADC_CHANNEL_0, &raw) == ESP_OK) {
        int pct = (raw - 2500) * 100 / 1700;
        if (pct < 0) pct = 0;
        if (pct > 100) pct = 100;
        return pct;
    }
    return 0;
}

static void set_led(uint8_t r, uint8_t g, uint8_t b)
{
    if (led) { led_strip_set_pixel(led, 0, r, g, b); led_strip_refresh(led); }
}

static void buzzer(uint32_t ms)
{
    gpio_set_level(APP_BUZZER_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(ms));
    gpio_set_level(APP_BUZZER_GPIO, 0);
}

static void print_packet(const char *type, const char *payload, const char *priority)
{
    char json[ESP_NOW_MAX_DATA_LEN];
    int n = snprintf(json, sizeof(json), "{\"protocol_version\":1,\"message_id\":\"%" PRIu32 "\",\"source_id\":\"%s\",\"destination_id\":\"gateway\",\"message_type\":\"%s\",\"timestamp\":0,\"ttl\":%d,\"hop_count\":0,\"priority\":\"%s\",\"payload\":%s}", sequence++, APP_DEVICE_ID, type, APP_TTL_DEFAULT, priority, payload);
    if (n < 0 || n >= (int)sizeof(json)) {
        ESP_LOGE(TAG, "TX_RESULT type=%s status=PACKET_TOO_LARGE bytes=%d", type, n);
        return;
    }
    last_tx_type = type;
    if (!gateway_seen) {
        ESP_LOGW(TAG, "TX_RESULT type=%s status=NO_GATEWAY bytes=%d", type, n);
        return;
    }
    esp_err_t err = esp_now_send(gateway_mac, (const uint8_t *)json, n);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "TX_RESULT type=%s status=QUEUE_FAILED error=%s", type, esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "TX_QUEUED type=%s priority=%s bytes=%d", type, priority, n);
    }
}

static void send_heartbeat(void)
{
    char p[96];
    snprintf(p, sizeof(p), "{\"status\":\"ONLINE\",\"uptime\":%" PRIu32 "}",
             (uint32_t)(esp_timer_get_time() / 1000000));
    print_packet("HEARTBEAT", p, "LOW");
}

static void send_telemetry(void)
{
    char p[192];
    snprintf(p, sizeof(p), "{\"battery\":%d,\"rssi\":%d,\"firmware_version\":\"%s\",\"uptime\":%" PRIu32 "}",
             battery_percent(), last_rssi, APP_FIRMWARE_VERSION,
             (uint32_t)(esp_timer_get_time() / 1000000));
    print_packet("TELEMETRY", p, "LOW");
}

static void send_sos(const char *reason)
{
    char p[192];
    if (!strcmp(reason, "INACTIVITY")) {
        snprintf(p, sizeof(p), "{\"reason\":\"INACTIVITY\",\"inactive_for_sec\":%d,\"inactivity_timeout_sec\":%d,\"battery\":%d,\"rssi\":%d}", APP_INACTIVITY_TIMEOUT_SEC, APP_INACTIVITY_TIMEOUT_SEC, battery_percent(), last_rssi);
    } else {
        snprintf(p, sizeof(p), "{\"reason\":\"MANUAL_SOS\",\"battery\":%d,\"rssi\":%d}", battery_percent(), last_rssi);
    }
    sos_active = true;
    set_led(255, 0, 0);
    buzzer(500);
    print_packet("SOS", p, "CRITICAL");
}

static void send_fall(void)
{
    char p[96];
    snprintf(p, sizeof(p), "{\"reason\":\"FALL_CONFIRMED\",\"cancelled\":false}");
    ESP_LOGW(TAG, "FALL_ALERT created immediately for test mode");
    print_packet("FALL_ALERT", p, "CRITICAL");
    fall_pending = false;
}

static void init_hardware(void)
{
    gpio_config_t out = {.pin_bit_mask=(1ULL<<APP_BUZZER_GPIO), .mode=GPIO_MODE_OUTPUT}; gpio_config(&out);
    gpio_config_t button = {.pin_bit_mask=(1ULL<<APP_SOS_BUTTON_GPIO), .mode=GPIO_MODE_INPUT, .pull_up_en=GPIO_PULLUP_ENABLE}; gpio_config(&button);
    i2c_master_bus_config_t bus_cfg = {.i2c_port=I2C_NUM_0, .sda_io_num=APP_I2C_SDA_GPIO, .scl_io_num=APP_I2C_SCL_GPIO, .clk_source=I2C_CLK_SRC_DEFAULT, .glitch_ignore_cnt=7, .flags.enable_internal_pullup=true};
    i2c_master_bus_handle_t bus;
    if (i2c_new_master_bus(&bus_cfg, &bus) == ESP_OK) {
        for (uint8_t address = 0x68; address <= 0x69 && !imu_ok; ++address) {
            if (i2c_master_probe(bus, address, 100) != ESP_OK) continue;
            i2c_device_config_t dc = {.dev_addr_length=I2C_ADDR_BIT_LEN_7, .device_address=address, .scl_speed_hz=400000};
            i2c_master_dev_handle_t candidate = NULL;
            if (i2c_master_bus_add_device(bus, &dc, &candidate) != ESP_OK) continue;
            uint8_t wake[2] = {0x6B, 0x00};
            uint8_t who = 0; uint8_t reg = 0x75;
            esp_err_t wake_err = i2c_master_transmit(candidate, wake, sizeof(wake), 100);
            esp_err_t who_err = i2c_master_transmit_receive(candidate, &reg, 1, &who, 1, 100);
            if (wake_err == ESP_OK && who_err == ESP_OK && (who == 0x68 || who == 0x69)) { imu = candidate; imu_ok = true; }
        }
    }
    ESP_LOGI(TAG, "SENSOR status=%s", imu_ok ? "READY" : "NOT_FOUND");
    adc_oneshot_unit_init_cfg_t ac = {.unit_id=ADC_UNIT_1}; adc_oneshot_new_unit(&ac, &adc);
    adc_oneshot_chan_cfg_t cc = {.atten=ADC_ATTEN_DB_12, .bitwidth=ADC_BITWIDTH_DEFAULT}; adc_oneshot_config_channel(adc, ADC_CHANNEL_0, &cc);
    led_strip_config_t lc = {.strip_gpio_num=APP_LED_GPIO, .max_leds=1, .led_model=LED_MODEL_WS2812, .color_component_format=LED_STRIP_COLOR_COMPONENT_FMT_GRB};
    led_strip_rmt_config_t rc = {.clk_src=RMT_CLK_SRC_DEFAULT, .resolution_hz=10*1000*1000};
    led_strip_new_rmt_device(&lc, &rc, &led); set_led(255, 0, 0);
}

static void init_radio(void)
{
    esp_netif_init(); esp_event_loop_create_default();
    wifi_init_config_t wc = WIFI_INIT_CONFIG_DEFAULT(); esp_wifi_init(&wc);
    esp_wifi_set_mode(WIFI_MODE_STA); esp_wifi_start(); esp_wifi_set_channel(APP_GATEWAY_CHANNEL, WIFI_SECOND_CHAN_NONE);
    esp_now_init(); esp_now_register_send_cb(on_espnow_send);
    uint8_t mac[6] = {0};
    if (strlen(APP_GATEWAY_MAC) == 17 && sscanf(APP_GATEWAY_MAC, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac[0],&mac[1],&mac[2],&mac[3],&mac[4],&mac[5]) == 6) {
        esp_now_peer_info_t peer = {.channel=APP_GATEWAY_CHANNEL, .ifidx=WIFI_IF_STA, .encrypt=false};
        memcpy(peer.peer_addr, mac, 6); memcpy(gateway_mac, mac, 6);
        esp_err_t peer_err = esp_now_add_peer(&peer); gateway_seen = peer_err == ESP_OK || peer_err == ESP_ERR_ESPNOW_EXIST;
        ESP_LOGI(TAG, "ESP-NOW peer %s", gateway_seen ? "ready" : "failed");
    } else ESP_LOGW(TAG, "Gateway MAC not configured; serial-only mode");
}

static void worker_task(void *arg)
{
    (void)arg;
    int64_t last_hb=0, last_tel=0; last_motion_ms=esp_timer_get_time()/1000;
    bool was_pressed=false; int64_t pressed_at=0;
    while (1) {
        int64_t now=esp_timer_get_time()/1000; int pressed = gpio_get_level(APP_SOS_BUTTON_GPIO)==0;
        if (pressed && !was_pressed) pressed_at=now;
        if (!pressed && was_pressed && now-pressed_at < APP_SOS_LONG_PRESS_MS && fall_pending) { fall_pending=false; set_led(0,0,0); ESP_LOGI(TAG,"Fall cancelled"); }
        if (pressed && now-pressed_at >= APP_SOS_LONG_PRESS_MS && !sos_active) send_sos("MANUAL_SOS");
        was_pressed=pressed;
        float ax,ay,az,temp;
        if (imu_sample(&ax,&ay,&az,&temp)) {
            float mag=sqrtf(ax*ax+ay*ay+az*az);
            if (fabsf(mag-1.0f)>APP_MOTION_DELTA_G) { last_motion_ms=now; inactivity_sent=false; if (fall_pending && !fall_alert_episode) fall_pending=false; if (fall_alert_episode && fabsf(mag-1.0f)>APP_MOTION_DELTA_G) fall_alert_episode=false; }
            if (!fall_pending && !fall_alert_episode && mag>APP_FALL_SPIKE_G) {
                fall_pending=true; fall_started_ms=now; set_led(255,180,0); buzzer(120);
                ESP_LOGW(TAG,"FALL SUSPECTED; immediate alert test=%d",APP_FALL_ALERT_ON_SUSPECT);
#if APP_FALL_ALERT_ON_SUSPECT
                send_fall();
                fall_alert_episode = true;
#endif
            }
        }
        if (!inactivity_sent && now-last_motion_ms >= APP_INACTIVITY_TIMEOUT_SEC*1000LL) { send_sos("INACTIVITY"); inactivity_sent=true; }
        if (fall_pending && now-fall_started_ms >= APP_FALL_CONFIRM_SEC*1000LL) send_fall();
        if (now-last_hb >= APP_HEARTBEAT_INTERVAL_SEC*1000LL) { send_heartbeat(); last_hb=now; }
        if (now-last_tel >= APP_TELEMETRY_INTERVAL_SEC*1000LL) { send_telemetry(); last_tel=now; }
        bool blink=(now/500)%2; if (fall_pending) set_led(blink?255:80, blink?160:50, 0); else if (sos_active) set_led(blink?255:80,0,0); else if (gateway_seen) set_led(0,blink?180:40,0); else set_led(blink?180:40,0,0);
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void app_start(void)
{
    esp_log_level_set("wifi", ESP_LOG_NONE); esp_log_level_set("i2c.master", ESP_LOG_NONE); esp_log_level_set("ESPNOW", ESP_LOG_NONE);
    esp_err_t nvs=nvs_flash_init(); if (nvs==ESP_ERR_NVS_NO_FREE_PAGES || nvs==ESP_ERR_NVS_NEW_VERSION_FOUND) { nvs_flash_erase(); nvs_flash_init(); }
    init_hardware(); init_radio(); ESP_LOGI(TAG,"MineMesh worker %s ready", APP_FIRMWARE_VERSION); xTaskCreate(worker_task,"worker",6144,NULL,5,NULL);
}
