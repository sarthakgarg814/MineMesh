#include "cli.h"
#include "wifi_service.h"
#include "mqtt_bridge.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "cli";
static SemaphoreHandle_t lock;
static char last_rx[251];
static char last_publish[251];
static void copy_payload(char *dst, const char *src, size_t len) { if (len > 250) len = 250; memcpy(dst, src, len); dst[len] = '\0'; }
void gateway_cli_set_last_rx(const char *data, size_t len) { if (lock && xSemaphoreTake(lock, pdMS_TO_TICKS(20)) == pdTRUE) { copy_payload(last_rx, data, len); xSemaphoreGive(lock); } }
void gateway_cli_set_last_publish(const char *data, size_t len) { if (lock && xSemaphoreTake(lock, pdMS_TO_TICKS(20)) == pdTRUE) { copy_payload(last_publish, data, len); xSemaphoreGive(lock); } }
static void print_help(void) { printf("help, status, wifi, mqtt, sniff, reboot\\n"); }
static void cli_task(void *arg) { (void)arg; char line[32]; print_help(); for (;;) { if (!fgets(line, sizeof(line), stdin)) { vTaskDelay(pdMS_TO_TICKS(250)); continue; } line[strcspn(line, "\\r\\n")] = '\0'; if (!strcmp(line, "help")) print_help(); else if (!strcmp(line, "status")) printf("wifi=%s mqtt=%s\\n", wifi_service_connected()?"up":"down", mqtt_bridge_connected()?"up":"down"); else if (!strcmp(line, "wifi")) printf("wifi=%s\\n", wifi_service_connected()?"connected":"disconnected"); else if (!strcmp(line, "mqtt")) printf("mqtt=%s\\n", mqtt_bridge_connected()?"connected":"disconnected"); else if (!strcmp(line, "sniff")) { if (lock && xSemaphoreTake(lock, pdMS_TO_TICKS(100)) == pdTRUE) { printf("last_rx=%s\\nlast_mqtt=%s\\n", last_rx[0]?last_rx:"<none>", last_publish[0]?last_publish:"<none>"); xSemaphoreGive(lock); } } else if (!strcmp(line, "reboot")) { ESP_LOGI(TAG, "reboot requested"); esp_restart(); } else if (line[0]) printf("unknown command: %s\\n", line); } }
void gateway_cli_start(void) { lock = xSemaphoreCreateMutex(); xTaskCreate(cli_task, "gateway_cli", 3072, NULL, 3, NULL); }
