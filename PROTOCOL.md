# MineMesh Protocol v1

UTF-8 JSON, one object per packet. No envelope fields may be omitted.

```json
{
  "protocol_version": 1,
  "message_id": "worker-001-42",
  "source_id": "worker-001",
  "destination_id": "gateway",
  "message_type": "HEARTBEAT|TELEMETRY|SOS|FALL_ALERT",
  "timestamp": 0,
  "ttl": 8,
  "hop_count": 0,
  "priority": "LOW|CRITICAL",
  "payload": {}
}
```

`message_id` is unique per device boot sequence. `ttl` defaults to 8 and packets with `hop_count >= ttl` must be discarded. `source_id` is `worker-001` for this device and destination is `gateway`.

## MQTT uplink contract

The gateway bridges valid worker ESP-NOW packets to MQTT topic `minemesh/v1/uplink`. The MQTT message body MUST be the same UTF-8 Protocol v1 JSON envelope received from the worker. The gateway must not wrap the envelope, rename fields, restructure `payload`, or replace `source_id` with the gateway identity. Implementations should publish the received bytes with their explicit length to preserve the original body; QoS 1 is the default and the message is non-retained. The ESP32 gateway is a bridge only and does not run an MQTT broker.

## Payloads

HEARTBEAT, LOW, every 10 seconds:

```json
{"status":"ONLINE","uptime":123}
```

TELEMETRY, LOW, every 15 seconds. Required fields are `battery` (0–100), `rssi` (dBm), `firmware_version`, and `uptime`. Optional fields are `temperature`, `mac_address`, and `free_heap`.

```json
{"battery":83,"rssi":-62,"firmware_version":"1.0.0","uptime":123,"temperature":28.4,"free_heap":182000}
```

SOS, CRITICAL:

```json
{"reason":"MANUAL_SOS","battery":83,"rssi":-62}
```

or:

```json
{"reason":"INACTIVITY","inactive_for_sec":60,"inactivity_timeout_sec":60,"battery":83,"rssi":-62}
```

FALL_ALERT, CRITICAL:

```json
{"reason":"FALL_CONFIRMED","countdown_sec (legacy optional field)":10,"cancelled":false,"battery":83,"rssi":-62}
```

Critical packets take precedence over low-priority packets and are retried by the worker transport. Receivers should deduplicate `message_id`. Packets are bounded by the ESP-NOW v1 maximum payload of 250 bytes.
