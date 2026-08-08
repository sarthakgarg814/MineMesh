#ifndef PROTOCOL_V1_H
#define PROTOCOL_V1_H
#include <stdbool.h>
#include <stddef.h>
typedef struct { char message_id[80]; char source_id[40]; char message_type[20]; char priority[12]; char critical_reason[32]; } protocol_meta_t;
bool protocol_v1_validate(const char *data, size_t len, const char *gateway_id, protocol_meta_t *meta);
#endif
