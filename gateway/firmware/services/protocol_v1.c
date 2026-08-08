#include "protocol_v1.h"
#include "app_config.h"
#include "cJSON.h"
#include "string.h"
#include "esp_log.h"
#include <stdlib.h>

static const char *TAG = "protocol";

static cJSON *obj(cJSON *root, const char *name)
{
    return cJSON_GetObjectItemCaseSensitive(root, name);
}

static bool string_field(cJSON *root, const char *name, char *out, size_t cap)
{
    cJSON *value = obj(root, name);
    if (!cJSON_IsString(value) || !value->valuestring || !value->valuestring[0] || strlen(value->valuestring) >= cap) {
        return false;
    }
    strlcpy(out, value->valuestring, cap);
    return true;
}

static bool integer_field(cJSON *root, const char *name, int *out)
{
    cJSON *value = obj(root, name);
    if (!cJSON_IsNumber(value) || value->valuedouble != (double)value->valueint) {
        return false;
    }
    *out = value->valueint;
    return true;
}

static bool validate_payload(cJSON *payload, const char *type, const char *priority,
                             char *reason, size_t reason_cap)
{
    if (!cJSON_IsObject(payload)) {
        return false;
    }

    if (!strcmp(type, "HEARTBEAT")) {
        cJSON *status = obj(payload, "status");
        int uptime;
        return cJSON_IsString(status) && !strcmp(status->valuestring, "ONLINE") &&
               integer_field(payload, "uptime", &uptime) && uptime >= 0;
    }

    if (!strcmp(type, "TELEMETRY")) {
        int battery;
        int rssi;
        int uptime;
        cJSON *firmware = obj(payload, "firmware_version");
        cJSON *temperature = obj(payload, "temperature");
        cJSON *mac = obj(payload, "mac_address");
        cJSON *heap = obj(payload, "free_heap");
        if (!integer_field(payload, "battery", &battery) || battery < 0 || battery > 100 ||
            !integer_field(payload, "rssi", &rssi) ||
            !integer_field(payload, "uptime", &uptime) || uptime < 0 ||
            !cJSON_IsString(firmware)) {
            return false;
        }
        return (!temperature || cJSON_IsNumber(temperature)) &&
               (!mac || cJSON_IsString(mac)) &&
               (!heap || cJSON_IsNumber(heap));
    }

    if (strcmp(priority, "CRITICAL")) {
        return false;
    }

    if (!strcmp(type, "SOS")) {
        cJSON *value = obj(payload, "reason");
        if (!cJSON_IsString(value) ||
            (strcmp(value->valuestring, "MANUAL_SOS") && strcmp(value->valuestring, "INACTIVITY"))) {
            return false;
        }
        if (!strcmp(value->valuestring, "INACTIVITY")) {
            int inactive;
            int timeout;
            if (!integer_field(payload, "inactive_for_sec", &inactive) || inactive < 0 ||
                !integer_field(payload, "inactivity_timeout_sec", &timeout) || timeout < 0) {
                return false;
            }
        }
        strlcpy(reason, value->valuestring, reason_cap);
        return true;
    }

    if (!strcmp(type, "FALL_ALERT")) {
        cJSON *value = obj(payload, "reason");
        cJSON *cancelled = obj(payload, "cancelled");
        if (!cJSON_IsString(value) || strcmp(value->valuestring, "FALL_CONFIRMED") ||
            !cJSON_IsFalse(cancelled)) {
            return false;
        }
        strlcpy(reason, value->valuestring, reason_cap);
        return true;
    }

    return false;
}

bool protocol_v1_validate(const char *data, size_t len, const char *gateway_id, protocol_meta_t *meta)
{
    if (!data || !meta || len == 0 || len > APP_ESPNOW_MAX_PAYLOAD) {
        return false;
    }
    char *copy = strndup(data, len);
    if (!copy) {
        return false;
    }
    cJSON *root = cJSON_ParseWithLength(copy, len);
    free(copy);
    if (!root || !cJSON_IsObject(root)) {
        cJSON_Delete(root);
        return false;
    }

    int version;
    int timestamp;
    int ttl;
    int hop;
    char destination[40];
    if (!integer_field(root, "protocol_version", &version) || version != 1 ||
        !integer_field(root, "timestamp", &timestamp) ||
        !integer_field(root, "ttl", &ttl) || ttl <= 0 ||
        !integer_field(root, "hop_count", &hop) || hop < 0 || hop >= ttl ||
        !string_field(root, "message_id", meta->message_id, sizeof(meta->message_id)) ||
        !string_field(root, "source_id", meta->source_id, sizeof(meta->source_id)) ||
        strcmp(meta->source_id, APP_WORKER_ID) ||
        !string_field(root, "destination_id", destination, sizeof(destination)) ||
        strcmp(destination, gateway_id) ||
        !string_field(root, "message_type", meta->message_type, sizeof(meta->message_type)) ||
        !string_field(root, "priority", meta->priority, sizeof(meta->priority))) {
        cJSON_Delete(root);
        return false;
    }

    const bool heartbeat = !strcmp(meta->message_type, "HEARTBEAT");
    const bool telemetry = !strcmp(meta->message_type, "TELEMETRY");
    const bool sos = !strcmp(meta->message_type, "SOS");
    const bool fall = !strcmp(meta->message_type, "FALL_ALERT");
    const bool known_type = heartbeat || telemetry || sos || fall;
    const bool low_priority_type = heartbeat || telemetry;
    const bool critical_priority_type = sos || fall;
    const bool priority_valid = (low_priority_type && !strcmp(meta->priority, "LOW")) ||
                                (critical_priority_type && !strcmp(meta->priority, "CRITICAL"));

    if (!known_type || !priority_valid ||
        !validate_payload(obj(root, "payload"), meta->message_type, meta->priority,
                          meta->critical_reason, sizeof(meta->critical_reason))) {
        cJSON_Delete(root);
        return false;
    }
    cJSON_Delete(root);
    ESP_LOGD(TAG, "Protocol v1 accepted");
    return true;
}
