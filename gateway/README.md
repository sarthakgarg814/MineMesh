# MineMesh Gateway Firmware 2

ESP-IDF 5.5 ESP32-C3 gateway image. This project is independent from `worker/` and contains no sensor, MPU6050, fall-detection, or SOS-generation logic.

## Build and flash

From the gateway directory:

```bash
idf.py set-target esp32c3
idf.py build
idf.py -p <PORT> flash monitor
```

The gateway uses the standard ESP-IDF station, ESP-NOW, and ESP-MQTT APIs. ESP-NOW and station Wi-Fi share one radio, so the worker must transmit on the access point's channel.

## Browser setup page

The gateway always includes the configuration page. On first boot, or when Wi-Fi or MQTT configuration is missing, it starts a setup access point:

- SSID: `MineMesh-Gateway`
- Password: `minemesh`
- Page: `http://192.168.4.1`

Enter the Wi-Fi SSID, Wi-Fi password, MQTT broker URI, topic, and gateway ID. Selecting **Save and reboot** stores the values in NVS and restarts the gateway. When the gateway is connected to station Wi-Fi, open the same page at its DHCP-assigned IP address. The page is HTTP-only and should be used only on a trusted local network. The setup password is an MVP default and should be changed in the source before deployment.

## Configuration

The `gateway` NVS namespace uses these keys:

- `wifi_ssid`
- `wifi_password`
- `mqtt_broker`, for example `mqtt://192.168.1.20:1883`
- `mqtt_topic_uplink` (default `minemesh/v1/uplink`)
- `gateway_id` (default `gateway`)

Credentials are intentionally not committed. Provision the NVS namespace with an ESP-IDF NVS CSV/image workflow before flashing, or add a provisioning command in your deployment environment.

## MQTT contract

The gateway publishes to `minemesh/v1/uplink` at QoS 1, non-retained. The MQTT body is the original worker Protocol v1 UTF-8 JSON byte sequence: it is not wrapped, renamed, or canonicalized.

Example body:

```json
{"protocol_version":1,"message_id":"worker-001-42","source_id":"worker-001","destination_id":"gateway","message_type":"SOS","timestamp":0,"ttl":8,"hop_count":0,"priority":"CRITICAL","payload":{"reason":"MANUAL_SOS","battery":83,"rssi":-62}}
```

The gateway validates the envelope, suppresses duplicate `message_id` values, and only retries critical SOS/FALL_ALERT messages when MQTT is unavailable.

## Serial commands

`help`, `status`, `wifi`, `mqtt`, `sniff`, and `reboot` are available on the UART console. `sniff` prints the last received and published raw JSON bodies. Logs include ESP-NOW reception and MQTT publish state.

## Worker dashboard and RGB LED

The browser page includes a live worker dashboard at `/api/workers`, refreshed every five seconds. It tracks up to 16 validated workers, marks workers online for 30 seconds after their last valid packet, and shows worker ID, online/offline status, last-seen age, message type, priority, and RSSI. This is gateway-local telemetry and is never added to MQTT payloads.

Press the BOOT button on GPIO9 once to erase the gateway Wi-Fi/MQTT configuration and reboot into the setup AP. This clears only the `gateway` NVS namespace; it does not erase the application or entire flash.

The gateway prints its actual STA MAC and Wi-Fi channel after station startup, for example `Gateway STA MAC: AA:BB:CC:DD:EE:FF channel=1`. Copy that exact MAC into the worker's `APP_GATEWAY_MAC` and set the worker's `APP_GATEWAY_CHANNEL` to the printed channel. The onboard RGB LED uses GPIO8. Blue indicates AP provisioning, amber indicates Wi-Fi startup, green indicates MQTT mode, cyan double-pulse indicates ESP-NOW reception, white double-pulse indicates MQTT transmission, red indicates errors, and rapid red pulses indicate SOS/FALL_ALERT activity. LED activity is non-blocking and does not alter the Protocol v1 bridge.
