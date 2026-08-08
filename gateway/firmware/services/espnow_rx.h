#ifndef ESPNOW_RX_H
#define ESPNOW_RX_H
#include <stddef.h>
#include <stdbool.h>
void espnow_rx_start(void);
bool espnow_rx_receive(char *data, size_t *len, int *rssi, size_t capacity);
#endif
