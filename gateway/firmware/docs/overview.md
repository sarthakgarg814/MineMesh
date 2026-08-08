# MineMesh Gateway Firmware 2

## Runtime status

The ESP32-C3 gateway is running in persisted AP + ESP-NOW receive-only mode. It starts `MineMesh-Gateway` with password `minemesh`, uses channel 1, and keeps Station Wi-Fi association and MQTT disabled until Wi-Fi + MQTT bridge mode is selected in the web UI.

```text
AP IP: 192.168.4.1
AP MAC: A0:F2:62:01:17:31
ESP-NOW channel: 1
```

The gateway RGB LED initialized successfully on GPIO8. Its physical illumination still requires visual confirmation on the board.

## Verified worker reception

The latest runtime test received valid canonical Protocol v1 packets:

```text
TELEMETRY priority=LOW RSSI=-19
HEARTBEAT priority=LOW RSSI=-20
HEARTBEAT priority=LOW RSSI=-28
```

The worker `worker-001` is therefore being discovered and tracked by the gateway. The dashboard is available at `http://192.168.4.1` after joining the `MineMesh-Gateway` AP.

## Protocol and alerts

The gateway validates the complete Protocol v1 envelope and type-specific payload schemas. HEARTBEAT and TELEMETRY require LOW priority; SOS and FALL_ALERT require CRITICAL priority. Valid critical packets are deduplicated, stored in a RAM-only newest-first history of ten entries, and shown in the dashboard. SOS uses a repeating magenta double-flash; FALL_ALERT uses a repeating red triple-flash. The UI can acknowledge the active alert without deleting history.

## Worker configuration

```c
#define APP_GATEWAY_MAC "A0:F2:62:01:17:31"
#define APP_GATEWAY_CHANNEL 1
```

## UI and build

The dashboard provides mode selection, Wi-Fi/MQTT configuration, worker status, RSSI, and critical-alert history. Select Wi-Fi + MQTT bridge mode and save to reboot into normal MQTT forwarding.

```text
idf.py set-target esp32c3
idf.py build
idf.py -p <PORT> flash monitor
```
