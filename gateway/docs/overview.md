# MineMesh Gateway Firmware 2

## Purpose

This ESP32-C3 gateway receives MineMesh Protocol v1 JSON envelopes from worker devices over ESP-NOW and forwards the unchanged UTF-8 JSON body to the MQTT uplink topic `minemesh/v1/uplink`. It validates the envelope, suppresses duplicate `message_id` values, and retains critical SOS/FALL_ALERT frames for retry when MQTT is unavailable. It does not contain sensors, MPU6050 code, fall detection, or SOS generation.

## Configuration and interfaces

Wi-Fi and MQTT settings are loaded from the `gateway` NVS namespace: `wifi_ssid`, `wifi_password`, `mqtt_broker`, `mqtt_topic_uplink`, and `gateway_id`. The default MQTT topic and gateway ID are `minemesh/v1/uplink` and `gateway`. ESP-NOW uses the station's current Wi-Fi channel, so workers must transmit on the access point channel.

The UART CLI supports `help`, `status`, `wifi`, `mqtt`, `sniff`, and `reboot`. The optional status LED is assigned to GPIO8. The MQTT body is published with explicit length and QoS 1 without wrapping or restructuring the worker envelope.

## Build and operation

```text
idf.py set-target esp32c3
idf.py build
idf.py -p <PORT> flash monitor
```

Provision Wi-Fi and MQTT values into the gateway NVS namespace before expecting network connections. The current hardware run confirmed clean startup and correctly reported that empty configuration was present; it did not attempt Wi-Fi or MQTT until credentials and broker URI are provisioned.
