#include <Arduino.h>
#include "../include/config.h"
#include "../include/pins.h"
#include "gps.h"
#include "lora.h"
#include "payload.h"
#include "display.h"
#include "nvs.h"

// Application state
enum AppState {
    STATE_INIT,
    STATE_JOIN,
    STATE_JOINED,
    STATE_GPS_WAIT,
    STATE_GPS_FIX,
    STATE_TRANSMIT,
    STATE_SLEEP,
    STATE_ERROR
};

AppState currentState = STATE_INIT;
uint32_t lastTransmitTime = 0;
uint32_t cycleCount = 0;
GPSData lastValidGPSData;  // Store last valid GPS data for transmission

// Function declarations
void initializeHardware();
void handleState();
void performTransmissionCycle();
void enterDeepSleep(uint32_t seconds);
void blinkLED(uint8_t count);

void setup() {
    // Initialize serial for debugging
    #if DEBUG_SERIAL
    Serial.begin(DEBUG_BAUD_RATE);
    delay(2000);  // Wait for serial connection
    Serial.println(F("\n\n"));
    Serial.println(F("========================================"));
    Serial.println(F("  T-Echo TTNMapper GPS Tracker"));
    Serial.println(F("========================================"));
    Serial.print(F("Firmware Version: 1.0.0\n"));
    Serial.print(F("Build Date: "));
    Serial.print(__DATE__);
    Serial.print(F(" "));
    Serial.println(__TIME__);
    Serial.println(F("========================================\n"));
    #endif
    
    // Initialize hardware (GPS and LoRa)
    initializeHardware();
    
    // Attempt LoRa join BEFORE initializing display (display is slow)
    #if DEBUG_SERIAL
    Serial.println(F("[Main] Attempting LoRaWAN join..."));
    Serial.flush();
    #endif
    
    if (loraModule.join(MAX_JOIN_RETRIES)) {
        #if DEBUG_SERIAL
        Serial.println(F("[Main] ✓ LoRaWAN join successful!"));
        Serial.flush();
        #endif
        currentState = STATE_JOINED;
    } else {
        #if DEBUG_SERIAL
        Serial.println(F("[Main] ✗ LoRaWAN join failed"));
        #endif
        currentState = STATE_JOIN;  // Will retry in loop
    }
    
    // Now initialize display (slow e-ink refresh)
    #if DEBUG_SERIAL
    Serial.println(F("[Main] Initializing display..."));
    Serial.flush();
    #endif
    
    if (DISPLAY_ENABLED) {
        display.begin();
        if (currentState == STATE_JOINED) {
            display.showJoined();
        } else {
            display.showStartup();
        }
    }
    
    #if DEBUG_SERIAL
    Serial.println(F("[Main] Setup complete, entering main loop"));
    Serial.println(F("[Main] Current state: "));
    Serial.println(currentState);
    Serial.flush();
    #endif
}

void loop() {
    handleState();
}

void initializeHardware() {
    #if DEBUG_SERIAL
    Serial.println(F("[Init] Initializing hardware..."));
    #endif
    
    // Configure LEDs
    pinMode(LED_RED, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_BLUE, OUTPUT);
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_BLUE, LOW);
    
    // Power enable
    pinMode(PIN_POWER_EN, OUTPUT);
    digitalWrite(PIN_POWER_EN, HIGH);
    
    // Blink to show we're alive
    blinkLED(3);
    
    // Initialize NVS (for storing LoRaWAN DevNonce across reboots)
    if (!nvsStorage.begin()) {
        #if DEBUG_SERIAL
        Serial.println(F("[Init] ⚠ NVS initialization failed (DevNonce won't persist)"));
        #endif
        // Continue anyway - NVS is optional
    }
    
    // Initialize GPS first
    if (!gpsModule.begin()) {
        #if DEBUG_SERIAL
        Serial.println(F("[Init] ✗ GPS initialization failed!"));
        #endif
        currentState = STATE_ERROR;
        return;
    }
    
    // Initialize LoRa before display (display is slow/blocking)
    if (!loraModule.begin()) {
        #if DEBUG_SERIAL
        Serial.println(F("[Init] ✗ LoRa initialization failed!"));
        #endif
        currentState = STATE_ERROR;
        return;
    }
    
    #if DEBUG_SERIAL
    Serial.println(F("[Init] ✓ Hardware initialized successfully\n"));
    #endif
}

void handleState() {
    switch (currentState) {
        case STATE_INIT:
            // Should not reach here, handled in setup()
            currentState = STATE_JOIN;
            break;
            
        case STATE_JOIN:
            #if DEBUG_SERIAL
            Serial.println(F("\n[State] JOINING TTN..."));
            #endif
            
            // Attempt OTAA join
            display.showJoining(1, MAX_JOIN_RETRIES);
            
            if (loraModule.join(MAX_JOIN_RETRIES)) {
                #if DEBUG_SERIAL
                Serial.println(F("[State] ✓ Join successful!\n"));
                #endif
                
                display.showJoined();
                delay(2000);
                
                currentState = STATE_JOINED;
            } else {
                #if DEBUG_SERIAL
                Serial.println(F("[State] ✗ Join failed after all retries"));
                Serial.println(F("[State] Restarting in 30 seconds...\n"));
                #endif
                
                display.showJoinFailed();
                delay(30000);
                
                // Restart
                NVIC_SystemReset();
            }
            break;
            
        case STATE_JOINED:
            #if DEBUG_SERIAL
            Serial.println(F("\n[State] JOINED - Starting transmission cycle"));
            Serial.flush();
            #endif
            
            currentState = STATE_GPS_WAIT;
            break;
            
        case STATE_GPS_WAIT:
            #if DEBUG_SERIAL
            Serial.println(F("\n[State] GPS_WAIT - Acquiring fix..."));
            Serial.flush();
            #endif
            
            display.showGPSSearching();
            
            #if DEBUG_SERIAL
            Serial.println(F("[State] Display updated, waking GPS..."));
            Serial.flush();
            #endif
            
            // Wake up GPS
            gpsModule.wakeup();
            delay(100);
            
            // Wait for GPS fix
            if (gpsModule.waitForFix(GPS_FIX_TIMEOUT_MS)) {
                lastValidGPSData = gpsModule.getData();  // Store the valid GPS data
                
                #if DEBUG_SERIAL
                Serial.println(F("[State] ✓ GPS fix acquired"));
                Serial.print(F("[State] Stored GPS: "));
                Serial.print(lastValidGPSData.latitude, 6);
                Serial.print(F(", "));
                Serial.print(lastValidGPSData.longitude, 6);
                Serial.print(F(" Valid: "));
                Serial.println(lastValidGPSData.valid ? "YES" : "NO");
                Serial.flush();
                #endif
                
                // Go straight to transmit (skip slow display update)
                currentState = STATE_TRANSMIT;
            } else {
                #if DEBUG_SERIAL
                Serial.println(F("[State] ✗ GPS fix timeout - skipping uplink\n"));
                #endif
                
                // Skip transmission, go to sleep
                currentState = STATE_SLEEP;
            }
            break;
            
        case STATE_GPS_FIX:
            #if DEBUG_SERIAL
            Serial.println(F("\n[State] GPS_FIX - Preparing transmission\n"));
            #endif
            
            currentState = STATE_TRANSMIT;
            break;
            
        case STATE_TRANSMIT: {
            #if DEBUG_SERIAL
            Serial.println(F("\n[State] TRANSMIT - Sending to TTN\n"));
            #endif
            
            cycleCount++;
            
            // Use the stored GPS data from GPS_WAIT state
            GPSData gpsData = lastValidGPSData;
            
            #if DEBUG_SERIAL
            Serial.print(F("[State] Using stored GPS: "));
            Serial.print(gpsData.latitude, 6);
            Serial.print(F(", "));
            Serial.print(gpsData.longitude, 6);
            Serial.print(F(" Valid: "));
            Serial.println(gpsData.valid ? "YES" : "NO");
            Serial.flush();
            #endif
            
            // Only send if we have valid GPS data
            if (!gpsData.valid) {
                #if DEBUG_SERIAL
                Serial.println(F("[State] ✗ GPS data invalid - skipping uplink\n"));
                #endif
                currentState = STATE_SLEEP;
                break;
            }
            
            // Encode GPS payload
            uint8_t payload[TTNMAPPER_PAYLOAD_SIZE];
            uint8_t payloadLen = payloadEncoder.encode(gpsData, payload);
            
            if (payloadLen == 0) {
                #if DEBUG_SERIAL
                Serial.println(F("[State] ✗ Payload encoding failed\n"));
                #endif
                currentState = STATE_SLEEP;
                break;
            }
            
            // Show transmission on display
            display.showTransmitting(cycleCount);
            
            // Send uplink
            digitalWrite(LED_BLUE, HIGH);
            bool success = loraModule.sendUplink(payload, payloadLen, TTNMAPPER_PORT, LORAWAN_CONFIRMED);
            digitalWrite(LED_BLUE, LOW);
            
            if (success) {
                #if DEBUG_SERIAL
                Serial.println(F("[State] ✓ Transmission successful\n"));
                #endif
                
                blinkLED(2);  // 2 blinks = success
                lastTransmitTime = millis();
            } else {
                #if DEBUG_SERIAL
                Serial.println(F("[State] ✗ Transmission failed\n"));
                #endif
                
                // Blink red LED on failure
                for (int i = 0; i < 3; i++) {
                    digitalWrite(LED_RED, HIGH);
                    delay(200);
                    digitalWrite(LED_RED, LOW);
                    delay(200);
                }
            }
            
            // Update status display
            display.showStatus(gpsData, loraModule.getState(), cycleCount);
            
            // Move to sleep state
            currentState = STATE_SLEEP;
            break;
        }
            
        case STATE_SLEEP:
            #if DEBUG_SERIAL
            Serial.print(F("\n[State] SLEEP - Next transmission in "));
            Serial.print(TX_INTERVAL_MS / 1000);
            Serial.println(F(" seconds\n"));
            Serial.flush();
            #endif
            
            // Put GPS to sleep to save power
            gpsModule.sleep();
            
            // Put display to sleep
            display.sleep();
            
            // Wait for next cycle
            delay(TX_INTERVAL_MS);
            
            // Start new cycle
            currentState = STATE_GPS_WAIT;
            break;
            
        case STATE_ERROR:
            #if DEBUG_SERIAL
            Serial.println(F("\n[State] ERROR - Fatal error occurred\n"));
            #endif
            
            display.showError("System Error");
            
            // Blink red LED continuously
            while (1) {
                digitalWrite(LED_RED, HIGH);
                delay(500);
                digitalWrite(LED_RED, LOW);
                delay(500);
            }
            break;
    }
}

void enterDeepSleep(uint32_t seconds) {
    // Note: This is a placeholder for deep sleep implementation
    // The nRF52840 requires setting up RTC and using the Nordic SDK
    // For now, we'll use a simple delay
    
    #if DEBUG_SERIAL
    Serial.println(F("[Sleep] Entering sleep mode..."));
    Serial.flush();
    #endif
    
    // Simple delay instead of deep sleep (for compatibility)
    delay(seconds * 1000);
    
    #if DEBUG_SERIAL
    Serial.println(F("[Sleep] Waking up..."));
    #endif
}

void blinkLED(uint8_t count) {
    for (uint8_t i = 0; i < count; i++) {
        digitalWrite(LED_GREEN, HIGH);
        delay(100);
        digitalWrite(LED_GREEN, LOW);
        delay(100);
    }
}
