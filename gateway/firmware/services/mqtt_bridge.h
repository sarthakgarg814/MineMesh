#ifndef MQTT_BRIDGE_H
#define MQTT_BRIDGE_H
#include <stdbool.h>
void mqtt_bridge_start(const char *uri,const char *topic);
bool mqtt_bridge_publish(const char *payload,int len);
bool mqtt_bridge_connected(void);
#endif
