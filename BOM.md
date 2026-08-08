# MineMesh — Bill of Materials (BOM)

MVP hardware for **one worker** + **one gateway**. Pin / GPIO assignments match `worker/firmware/configs/app_config.h` and `gateway/firmware/configs/app_config.h`. See [docs/wiring.md](./docs/wiring.md) for connection diagrams.

Prices are approximate hackathon / hobbyist ranges (USD) and will vary by vendor and region.

---

## Worker node (Firmware 1)

| Qty | Part | Spec / notes | Est. unit |
|---:|---|---|---:|
| 1 | ESP32-C3 board | DevKit-style with USB-C, on-board WS2812 on **GPIO8** preferred | $4–8 |
| 1 | MPU6050 module | 6-axis IMU, I2C addr **0x68** (AD0 → GND); SDA **GPIO4**, SCL **GPIO5** | $2–4 |
| 1 | Tactile push button | SOS, active-low to **GPIO9** (internal pull-up) | $0.20 |
| 1 | Active buzzer (5 V or 3.3 V) | Drive from **GPIO6** (use transistor if module needs more current) | $0.50–1 |
| 1 | Battery pack | LiPo / 1S with USB charge board for wearable demo | $3–8 |
| 1 | Voltage divider (2× resistor) | Battery sense into **GPIO0** (ADC1 CH0); size for ≤ ~3.3 V at ADC | $0.10 |
| — | Dupont wires / breadboard | Prototyping | $2 |
| — | Optional: NPN + 1k base resistor | If buzzer current exceeds GPIO limit | $0.20 |

**Worker subtotal (typical):** ~$12–25

### Worker GPIO summary

| Function | GPIO |
|---|---:|
| MPU6050 SDA | 4 |
| MPU6050 SCL | 5 |
| SOS button | 9 |
| Buzzer | 6 |
| Battery ADC | 0 |
| RGB LED (on-board WS2812) | 8 |

Do **not** use ESP32-C3 GPIO 11–17 (flash).

---

## Gateway node (Firmware 2)

| Qty | Part | Spec / notes | Est. unit |
|---:|---|---|---:|
| 1 | ESP32-C3 board | Wi-Fi capable; USB power is enough (no MPU6050) | $4–8 |
| — | On-board RGB LED | Status on **GPIO8** (same family boards as worker) | included |
| — | BOOT button | On-board **GPIO9** — factory-reset gateway NVS in firmware | included |
| 1 | USB cable / power supply | 5 V USB | $2–5 |

**Gateway subtotal (typical):** ~$6–13

No IMU, buzzer, or battery sense required on the gateway for the MVP.

---

## Shared / bench (demo)

| Qty | Part | Spec / notes | Est. unit |
|---:|---|---|---:|
| 1 | Laptop or SBC | Runs Mosquitto (optional MQTT Path B) + serial monitor | — |
| 1 | USB hub / 2× USB ports | Flash + power both boards | $5–10 |
| — | Phone / laptop Wi-Fi | Join `MineMesh-Gateway` SoftAP for local dashboard | — |

---

## Full MVP kit totals

| Kit | Boards | Approx. parts cost |
|---|---|---|
| 1 worker + 1 gateway | 2× ESP32-C3 + 1× MPU6050 + button + buzzer + battery sense | **~$20–40** |
| +2 extra workers (same BOM as worker row) | scale without new cellular SIMs | +~$12–25 each |

Optional later (not required by current firmware): vibration motor, fuel-gauge IC, OLED, enclosure / helmet mount, level shifter (if using 5 V I2C modules incorrectly — prefer 3.3 V modules).

---

## Notes

- Prefer **3.3 V** MPU6050 modules; ESP32-C3 I/O is not 5 V tolerant.
- SOS button: one side to **GPIO9**, other to **GND**; firmware enables internal pull-up.
- Battery ADC on **GPIO0** needs a safe divider so the pin never exceeds 3.3 V.
- Gateway and worker must share the same ESP-NOW **channel**; worker `APP_GATEWAY_MAC` must match the gateway STA/AP MAC printed at boot.

Wiring diagrams: [`docs/wiring.md`](./docs/wiring.md).
