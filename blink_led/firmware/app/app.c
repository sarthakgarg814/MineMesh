#include "app.h"

#include <stdbool.h>

#include "app_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "logger.h"
#include "rgb_led.h"

static const char *TAG = "app";

void app_start(void)
{
    rgb_led_init();
    ESP_LOGI(TAG, "firmware started; blinking every %d ms", APP_BLINK_PERIOD_MS);

    bool led_on = false;
    while (true) {
        led_on = !led_on;
        rgb_led_set(led_on);
        ESP_LOGI(TAG, "LED %s", led_on ? "on" : "off");
        vTaskDelay(pdMS_TO_TICKS(APP_BLINK_PERIOD_MS));
    }
}
