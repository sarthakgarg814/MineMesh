# MineMesh Worker Firmware

ESP-IDF firmware for the MineMesh underground worker end device. It emits MineMesh Protocol v1 JSON packets to the serial console and, when a gateway MAC is configured, over ESP-NOW.

## Build and flash

```bash
idf.py set-target esp32c3
idf.py build
idf.py -p PORT flash monitor
```

The firmware uses `worker-001` and is serial-only until `APP_GATEWAY_MAC` in `firmware/configs/app_config.h` is set to the gateway's MAC address. ESP-NOW channel defaults to 1.

## Pin map

| Function | GPIO |
|---|---:|
| MPU6050 SDA | 4 |
| MPU6050 SCL | 5 |
| MPU6050 address | 0x68 |
| SOS button (active low, pull-up) | 9 |
| Buzzer | 6 |
| Battery ADC (ADC1) | 0 |
| On-board RGB LED (WS2812) | 8 |

Do not use ESP32-C3 GPIO 11–17; they are connected to flash.

## Behavior

Heartbeat is sent every 10 seconds and telemetry every 15 seconds. A two-second button hold sends manual SOS. Inactivity sends one SOS after 60 seconds without meaningful acceleration change and re-arms after movement. A fall spike starts a ten-second yellow countdown; a short button press cancels it, otherwise `FALL_ALERT` is sent. SOS is red, fall is yellow, gateway-connected is green blinking, and gateway-disconnected is red blinking. The MPU6050 is abstracted by the sensor read functions and sensor-derived alerts are suppressed when initialization fails.

## Serial commands

The supported command vocabulary is: `help`, `status`, `sos`, `sos inactivity`, `fall`, `telemetry`, `neighbors`, `config`, `config set inactivity_timeout_sec <n>`, and `reboot`. Every packet is printed as one JSON line. `timestamp` is Unix time when the system clock is set, otherwise the ESP-IDF epoch value is used.

## Example

```json
{"protocol_version":1,"message_id":"worker-001-42","source_id":"worker-001","destination_id":"gateway","message_type":"SOS","timestamp":0,"ttl":8,"hop_count":0,"priority":"CRITICAL","payload":{"reason":"MANUAL_SOS","battery":83,"rssi":-62}}
```
