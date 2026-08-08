#include "boot_button.h"
#include "driver/gpio.h"
#include "app_config.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "boot_button";
static boot_button_reset_cb_t reset_cb;

static void button_task(void *arg)
{
    (void)arg;
    bool was_pressed = false;
    for (;;) {
        bool pressed = gpio_get_level(APP_BOOT_BUTTON_GPIO) == 0;
        if (pressed && !was_pressed) {
            vTaskDelay(pdMS_TO_TICKS(40));
            if (gpio_get_level(APP_BOOT_BUTTON_GPIO) == 0) {
                ESP_LOGW(TAG, "BOOT button pressed; resetting gateway configuration");
                if (reset_cb) reset_cb();
                vTaskDelete(NULL);
            }
        }
        was_pressed = pressed;
        vTaskDelay(pdMS_TO_TICKS(25));
    }
}

void boot_button_start(boot_button_reset_cb_t callback)
{
    reset_cb = callback;
    gpio_config_t config = {
        .pin_bit_mask = 1ULL << APP_BOOT_BUTTON_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&config));
    xTaskCreate(button_task, "boot_button", 2048, NULL, 4, NULL);
}
