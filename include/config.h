#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================
// TTN LoRaWAN Configuration (OTAA)
// ============================================
// IMPORTANT: Replace these placeholder values with your actual credentials from TTN Console!
// 
// How to get these values:
// 1. Create an application at https://console.cloud.thethings.network/
// 2. Register a new end device with LoRaWAN 1.0.x OTAA
// 3. Copy the JoinEUI (AppEUI), DevEUI, and AppKey
// 4. Use MSB format for all values

// JoinEUI (AppEUI) - 8 bytes in MSB format
// Can use all zeros for community network
extern uint64_t joinEUI;

// DevEUI - 8 bytes in MSB format
// MUST be unique for each device
// Example: 0x70B3D57ED005XXXX
extern uint64_t devEUI;

// AppKey - 16 bytes in MSB format
// Keep this secret! Generate on TTN Console
extern uint8_t appKey[16];

// Network Key (LoRaWAN 1.1 only, leave as-is for 1.0.x)
extern uint8_t nwkKey[16];

// ============================================
// Credential Definitions (edit your values here)
// ============================================
#ifdef CONFIG_IMPLEMENTATION

// JoinEUI (AppEUI) - 8 bytes as shown in TTN Console (MSB format)
// TTN Console shows: 00 00 00 00 00 00 00 00
uint64_t joinEUI = 0x0000000000000000;

// DevEUI - 8 bytes as shown in TTN Console (MSB format)
// TTN Console shows: 00 00 00 00 00 00 00 00
uint64_t devEUI = 0x0000000000000000;

// AppKey - 16 bytes in MSB format (as shown in TTN Console)
uint8_t appKey[16] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Network Key - For LoRaWAN 1.1.x (separate from AppKey)
uint8_t nwkKey[16] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

#endif // CONFIG_IMPLEMENTATION

// ============================================
// TTNMapper Configuration
// ============================================
// TTNMapper requires:
// - Payload with latitude, longitude, altitude, hdop
// - Uplink on any port (typically port 1)
// - Decoder configured in TTN Console (see ttn-decoder.js)
// - TTNMapper integration webhook configured

#define TTNMAPPER_PORT      1          // LoRaWAN uplink port

// ============================================
// Transmission Settings
// ============================================
#define TX_INTERVAL_MS      (60 * 1000)       // 60 seconds for testing
#define GPS_FIX_TIMEOUT_MS  15000             // 15 seconds for testing
#define MIN_SATELLITES      4                 // Minimum satellites for valid fix

// LoRaWAN settings
#define LORAWAN_DATARATE    5                 // SF7BW125 (fastest for EU868)
#define LORAWAN_TX_POWER    14                // dBm (max for EU868)
#define LORAWAN_CONFIRMED   false             // Use unconfirmed uplinks for TTNMapper

// Join retry settings
#define JOIN_RETRY_INTERVAL 120000            // 2 minutes between join attempts (preserves gateway duty cycle)
#define MAX_JOIN_RETRIES    10                // Maximum join attempts before reset

// ============================================
// GPS Settings
// ============================================
#define GPS_BAUD_RATE       9600
#define GPS_UPDATE_RATE     1000              // 1 Hz update rate

// ============================================
// Power Management
// ============================================
#define ENABLE_DEEP_SLEEP   true              // Use deep sleep between transmissions
#define BATTERY_CHECK       true              // Include battery voltage in payload

// ============================================
// Display Settings
// ============================================
#define DISPLAY_ENABLED     true              // Enable e-paper display updates
#define DISPLAY_ROTATION    3                 // 0, 1, 2, or 3 (90° increments) - 3 = 90° left

// ============================================
// Debug Settings
// ============================================
#define DEBUG_SERIAL        true              // Enable serial debug output
#define DEBUG_BAUD_RATE     115200

#endif // CONFIG_H
