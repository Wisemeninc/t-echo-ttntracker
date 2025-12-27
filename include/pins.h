#ifndef PINS_H
#define PINS_H

// Lilygo T-Echo Pin Definitions
// Reference: https://github.com/Xinyuan-LilyGO/T-Echo/blob/main/examples/SX126x_Transmit_Interrupt/utilities.h

#ifndef _PINNUM
#define _PINNUM(port, pin)    ((port)*32 + (pin))
#endif

// Power control
#define PIN_POWER_EN        _PINNUM(0, 12)    // VDD 3.3V enable (active high)

// LoRa SX1262 pins (uses NRF_SPIM3)
#define LORA_MISO           _PINNUM(0, 23)    // P0.23
#define LORA_MOSI           _PINNUM(0, 22)    // P0.22
#define LORA_SCLK           _PINNUM(0, 19)    // P0.19
#define LORA_CS             _PINNUM(0, 24)    // P0.24
#define LORA_DIO1           _PINNUM(0, 20)    // P0.20
#define LORA_RESET          _PINNUM(0, 25)    // P0.25
#define LORA_BUSY           _PINNUM(0, 17)    // P0.17

// E-Paper Display pins (uses NRF_SPIM2) - DIFFERENT SPI bus from LoRa!
#define EPD_MISO            _PINNUM(1, 6)     // P1.6 (not used, but defined)
#define EPD_MOSI            _PINNUM(0, 29)    // P0.29
#define EPD_SCLK            _PINNUM(0, 31)    // P0.31
#define EPD_CS              _PINNUM(0, 30)    // P0.30
#define EPD_DC              _PINNUM(0, 28)    // P0.28
#define EPD_RESET           _PINNUM(0, 2)     // P0.2
#define EPD_BUSY            _PINNUM(0, 3)     // P0.3
#define EPD_BACKLIGHT       _PINNUM(1, 11)    // P1.11

// GPS L76K pins (UART)
#define GPS_RX_PIN          _PINNUM(1, 9)     // P1.9
#define GPS_TX_PIN          _PINNUM(1, 8)     // P1.8
#define GPS_WAKEUP_PIN      _PINNUM(1, 2)     // P1.2
#define GPS_RESET_PIN       _PINNUM(1, 5)     // P1.5
#define GPS_PPS_PIN         _PINNUM(1, 4)     // P1.4 - Pulse Per Second

// I2C pins (RTC PCF8563, Sensor BMP280)
#define I2C_SDA             _PINNUM(0, 26)    // P0.26
#define I2C_SCL             _PINNUM(0, 27)    // P0.27

// Battery measurement
#define VBAT_PIN            _PINNUM(0, 4)     // P0.4 - Battery voltage (ADC)

// LEDs
#define LED_GREEN           _PINNUM(1, 1)     // P1.1
#define LED_RED             _PINNUM(1, 3)     // P1.3
#define LED_BLUE            _PINNUM(0, 14)    // P0.14

// Touch button
#define TOUCH_PIN           _PINNUM(0, 11)    // P0.11

// User button
#define USER_BUTTON_PIN     _PINNUM(1, 10)    // P1.10

// QSPI Flash
#define FLASH_CS            _PINNUM(1, 15)    // P1.15
#define FLASH_SCLK          _PINNUM(1, 14)    // P1.14
#define FLASH_MOSI          _PINNUM(1, 12)    // P1.12 (D0)
#define FLASH_MISO          _PINNUM(1, 13)    // P1.13 (D1)

// RTC interrupt
#define RTC_INT_PIN         _PINNUM(0, 16)    // P0.16

#endif // PINS_H
