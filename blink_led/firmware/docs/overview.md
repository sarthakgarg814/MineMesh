# Blink LED Firmware

This ESP-IDF firmware blinks the onboard addressable RGB LED on the ESP32-C3-DevKitM-1. The LED displays low-brightness white for 500 ms, then turns off for 500 ms, continuously.

## Hardware and configuration

- Board: ESP32-C3-DevKitM-1
- LED: onboard addressable RGB LED
- LED data GPIO: 8
- Blink interval: 500 ms
- LED brightness: 16/255 per RGB channel

Application constants are defined in `firmware/configs/app_config.h`. The platform-specific `led_strip` implementation is in `firmware/platforms/esp32/rgb_led.c`; application orchestration is in `firmware/app/app.c`.

## Build and flash

From the project directory, build with ESP-IDF, flash the connected ESP32-C3 board, and open the serial monitor at 115200 baud. The firmware logs initialization and each LED state transition.
