#ifndef GATEWAY_CLI_H
#define GATEWAY_CLI_H
#include <stddef.h>
void gateway_cli_start(void);
void gateway_cli_set_last_rx(const char *data, size_t len);
void gateway_cli_set_last_publish(const char *data, size_t len);
#endif
