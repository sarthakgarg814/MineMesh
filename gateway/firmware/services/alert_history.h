#ifndef ALERT_HISTORY_H
#define ALERT_HISTORY_H
#include <stdbool.h>
#include <stddef.h>
#include "protocol_v1.h"
void alert_history_init(void);
void alert_history_add(const char *raw, size_t len, const protocol_meta_t *meta);
size_t alert_history_json(char *out, size_t capacity);
void alert_history_acknowledge(void);
bool alert_history_active(void);
#endif
