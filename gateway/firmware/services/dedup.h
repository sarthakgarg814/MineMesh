#ifndef DEDUP_H
#define DEDUP_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
bool dedup_seen_or_add(const char *id);
bool retry_enqueue(const char *data, size_t len);
bool retry_dequeue(char *data, size_t *len, size_t capacity);
#endif
