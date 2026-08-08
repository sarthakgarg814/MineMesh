#include "rgb_led.h"

#include "app_config.h"
#include "esp_err.h"
#include "esp_log.h"
#include "led_strip.h"

static const char *TAG = "rgb_led";
static led_strip_handle_t s_led_strip;

void rgb_led_init(void)
{
    const led_strip_config_t strip_config = {
        .strip_gpio_num = APP_LED_GPIO,
        .max_leds = APP_LED_COUNT,
    };
    const led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000,
        .flags.with_dma = false,
    };

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &s_led_strip));
    ESP_ERROR_CHECK(led_strip_clear(s_led_strip));
    ESP_LOGI(TAG, "RGB LED initialized on GPIO %d", APP_LED_GPIO);
}

void rgb_led_set(bool on)
{
    if (on) {
        ESP_ERROR_CHECK(led_strip_set_pixel(s_led_strip, 0,
                                            APP_LED_BRIGHTNESS,
                                            APP_LED_BRIGHTNESS,
                                            APP_LED_BRIGHTNESS));
        ESP_ERROR_CHECK(led_strip_refresh(s_led_strip));
    } else {
        ESP_ERROR_CHECK(led_strip_clear(s_led_strip));
    }
}
