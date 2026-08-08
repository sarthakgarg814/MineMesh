#ifndef WORKER_STATUS_H
#define WORKER_STATUS_H
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#define WORKER_STATUS_MAX 16
#define WORKER_STATUS_ONLINE_MS 30000
typedef struct { char source_id[40]; char message_type[20]; char priority[12]; int rssi; uint32_t last_seen_ms; bool online; } worker_status_t;
void worker_status_init(void);
void worker_status_update(const char *source_id, const char *message_type, const char *priority, int rssi);
size_t worker_status_json(char *out, size_t capacity);
#endif
