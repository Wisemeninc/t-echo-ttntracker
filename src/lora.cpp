#include "lora.h"
#include "nvs.h"
#include "../include/pins.h"
#include "../include/config.h"

LoRaWANModule loraModule;

LoRaWANModule::LoRaWANModule() 
    : rfPort(nullptr),
      radioModule(nullptr),
      radio(nullptr),
      node(nullptr),
      state(LORA_NOT_JOINED),
      uplinkCount(0),
      lastRSSI(0),
      lastSNR(0) {
}

bool LoRaWANModule::begin() {
    #if DEBUG_SERIAL
    Serial.println(F("[LoRa] Initializing SX1262..."));
    #endif
    
    // Seed random number generator for DevNonce
    randomSeed(analogRead(0) ^ micros());
    
    // Configure all LoRa pins explicitly
    pinMode(LORA_CS, OUTPUT);
    digitalWrite(LORA_CS, HIGH);
    
    pinMode(LORA_RESET, OUTPUT);
    pinMode(LORA_BUSY, INPUT);
    pinMode(LORA_DIO1, INPUT_PULLDOWN);  // DIO1 needs pulldown for proper interrupt detection
    
    // Perform hardware reset
    #if DEBUG_SERIAL
    Serial.println(F("[LoRa] Hardware reset..."));
    #endif
    digitalWrite(LORA_RESET, LOW);
    delay(10);
    digitalWrite(LORA_RESET, HIGH);
    delay(10);
    
    // Wait for BUSY to go low
    while (digitalRead(LORA_BUSY) == HIGH) {
        delay(1);
    }
    
    #if DEBUG_SERIAL
    Serial.println(F("[LoRa] Creating SPI port on NRF_SPIM3..."));
    #endif
    
    // Create SPI port for LoRa (NRF_SPIM3) - exactly like official examples
    rfPort = new SPIClass(NRF_SPIM3, LORA_MISO, LORA_SCLK, LORA_MOSI);
    rfPort->begin();
    
    // Create Module with SPI settings
    SPISettings spiSettings;
    radioModule = new Module(LORA_CS, LORA_DIO1, LORA_RESET, LORA_BUSY, *rfPort, spiSettings);
    
    // Create radio
    radio = new SX1262(radioModule);
    
    // Create LoRaWAN node
    node = new LoRaWANNode(radio, &EU868);
    
    #if DEBUG_SERIAL
    Serial.println(F("[LoRa] Calling radio->begin()..."));
    #endif
    
    // Initialize radio - TCXO voltage MUST be set in begin() call for T-Echo
    // Using 1.8V TCXO for better frequency stability
    int16_t result = radio->begin(868.0, 125.0, 9, 7, RADIOLIB_SX126X_SYNC_WORD_PUBLIC, 14, 8, 1.8, false);
    if (result != RADIOLIB_ERR_NONE) {
        #if DEBUG_SERIAL
        Serial.print(F("[LoRa] Radio init failed, code: "));
        Serial.println(result);
        Serial.print(F("[LoRa] BUSY pin: "));
        Serial.println(digitalRead(LORA_BUSY) ? "HIGH" : "LOW");
        #endif
        return false;
    }
    
    // CRITICAL: Set TCXO voltage explicitly with longer stabilization time
    // T-Echo SX1262 needs adequate TCXO warm-up for accurate RX frequency
    // setTCXO(voltage, delay_us) - use 5000us (5ms) delay
    result = radio->setTCXO(1.8, 5000);
    if (result != RADIOLIB_ERR_NONE) {
        #if DEBUG_SERIAL
        Serial.print(F("[LoRa] TCXO config result: "));
        Serial.println(result);
        #endif
    }
    
    // Allow additional TCXO stabilization
    delay(20);
    
    // Set DIO2 as RF switch control (required for T-Echo)
    result = radio->setDio2AsRfSwitch(true);
    if (result != RADIOLIB_ERR_NONE) {
        #if DEBUG_SERIAL
        Serial.print(F("[LoRa] DIO2 RF switch config failed, code: "));
        Serial.println(result);
        #endif
    }
    
    // Explicitly set RX boosted gain for better sensitivity
    radio->setRxBoostedGainMode(true);
    
    // Set current limit
    radio->setCurrentLimit(140);
    
    #if DEBUG_SERIAL
    Serial.println(F("[LoRa] SX1262 initialized successfully"));
    Serial.println(F("[LoRa] TCXO: 1.6V, DIO2 RF switch, RX boost enabled"));
    Serial.print(F("[LoRa] DevEUI: "));
    Serial.println((unsigned long)(devEUI & 0xFFFFFFFF), HEX);
    #endif
    
    return true;
}

bool LoRaWANModule::configureRadio() {
    int16_t result;
    
    // Set frequency to 868.1 MHz (EU868 default channel)
    result = radio->setFrequency(868.1);
    if (result != RADIOLIB_ERR_NONE) return false;
    
    // Set output power to 14 dBm (max for EU868)
    result = radio->setOutputPower(LORAWAN_TX_POWER);
    if (result != RADIOLIB_ERR_NONE) return false;
    
    // Set bandwidth to 125 kHz
    result = radio->setBandwidth(125.0);
    if (result != RADIOLIB_ERR_NONE) return false;
    
    // Set spreading factor to 7 (SF7)
    result = radio->setSpreadingFactor(7);
    if (result != RADIOLIB_ERR_NONE) return false;
    
    // Set coding rate to 4/5
    result = radio->setCodingRate(5);
    if (result != RADIOLIB_ERR_NONE) return false;
    
    // Set sync word to 0x34 (LoRaWAN public network)
    result = radio->setSyncWord(0x34);
    if (result != RADIOLIB_ERR_NONE) return false;
    
    // Set preamble length to 8
    result = radio->setPreambleLength(8);
    if (result != RADIOLIB_ERR_NONE) return false;
    
    // Enable CRC
    result = radio->setCRC(true);
    if (result != RADIOLIB_ERR_NONE) return false;
    
    #if DEBUG_SERIAL
    Serial.println(F("[LoRa] Radio configured: EU868, SF7, BW125, 14dBm"));
    #endif
    
    return true;
}

bool LoRaWANModule::join(uint8_t maxRetries) {
    #if DEBUG_SERIAL
    Serial.println(F("[LoRa] Starting OTAA join..."));
    #endif
    
    state = LORA_JOINING;
    
    // Try to restore saved nonces (DevNonce) from NVS
    uint8_t noncesBuffer[RADIOLIB_LORAWAN_NONCES_BUF_SIZE];
    bool noncesRestored = false;
    
    if (nvsStorage.loadNonces(noncesBuffer, RADIOLIB_LORAWAN_NONCES_BUF_SIZE)) {
        #if DEBUG_SERIAL
        Serial.println(F("[LoRa] Attempting to restore saved nonces from NVS"));
        #endif
        node->beginOTAA(joinEUI, devEUI, appKey, appKey);
        int16_t restoreResult = node->setBufferNonces(noncesBuffer);
        #if DEBUG_SERIAL
        Serial.print(F("[LoRa] Nonces restore result: "));
        Serial.println(restoreResult);
        #endif
        
        // Only consider nonces restored if successful (0) or already have valid nonces
        if (restoreResult == RADIOLIB_ERR_NONE) {
            noncesRestored = true;
        } else {
            // Clear corrupted nonces and start fresh
            #if DEBUG_SERIAL
            Serial.println(F("[LoRa] Clearing corrupted nonces, starting fresh"));
            #endif
            nvsStorage.clearAll();
        }
    }
    
    for (uint8_t retry = 0; retry < maxRetries; retry++) {
        #if DEBUG_SERIAL
        Serial.print(F("[LoRa] Join attempt "));
        Serial.print(retry + 1);
        Serial.print(F("/"));
        Serial.println(maxRetries);
        #endif
        
        // Attempt OTAA join (beginOTAA returns void in RadioLib 6.6.0)
        // For LoRaWAN 1.0.x compatibility, use appKey for both nwkKey and appKey
        // For LoRaWAN 1.1.x, use: node->beginOTAA(joinEUI, devEUI, nwkKey, appKey);
        node->beginOTAA(joinEUI, devEUI, appKey, appKey);
        
        // Ensure DIO2 RF switch is set before TX and RX
        radio->setDio2AsRfSwitch(true);
        
        // Set RX boosted gain for better sensitivity during join-accept
        radio->setRxBoostedGainMode(true);
        
        #if DEBUG_SERIAL
        Serial.println(F("[LoRa] Sending join request, waiting for accept..."));
        
        // Print full 8-byte EUIs for verification
        Serial.print(F("[LoRa] JoinEUI: "));
        uint8_t* joinBytes = (uint8_t*)&joinEUI;
        for (int i = 7; i >= 0; i--) {
            if (joinBytes[i] < 0x10) Serial.print('0');
            Serial.print(joinBytes[i], HEX);
        }
        Serial.println();
        
        Serial.print(F("[LoRa] DevEUI:  "));
        uint8_t* devBytes = (uint8_t*)&devEUI;
        for (int i = 7; i >= 0; i--) {
            if (devBytes[i] < 0x10) Serial.print('0');
            Serial.print(devBytes[i], HEX);
        }
        Serial.println();
        
        Serial.print(F("[LoRa] NwkKey:  "));
        for (int i = 0; i < 4; i++) {
            if (nwkKey[i] < 0x10) Serial.print('0');
            Serial.print(nwkKey[i], HEX);
        }
        Serial.println(F("... (first 4 bytes)"));
        
        Serial.print(F("[LoRa] AppKey:  "));
        for (int i = 0; i < 4; i++) {
            if (appKey[i] < 0x10) Serial.print('0');
            Serial.print(appKey[i], HEX);
        }
        Serial.println(F("... (first 4 bytes, USING THIS FOR 1.0.x)"));
        
        Serial.println(F("[LoRa] Band: EU868, LoRaWAN 1.0.x mode"));
        Serial.println(F("[LoRa] Waiting for join-accept (RX1: 5s, RX2: 6s)..."));
        Serial.flush();
        #endif
        
        // Put radio in standby before activation to ensure clean state
        radio->standby();
        
        // Check if join was successful by attempting to activate
        // activateOTAA() sends join request and waits for accept
        int16_t result = node->activateOTAA();
        
        #if DEBUG_SERIAL
        // Debug RX state
        Serial.print(F("[LoRa] Post-RX BUSY: "));
        Serial.println(digitalRead(LORA_BUSY) ? "HIGH" : "LOW");
        Serial.print(F("[LoRa] Post-RX DIO1: "));
        Serial.println(digitalRead(LORA_DIO1) ? "HIGH" : "LOW");
        #endif
        
        // IMPORTANT: Save nonces after EVERY attempt (not just success)
        // RadioLib increments DevNonce after sending, so we must save it
        // even if we didn't receive the join-accept
        uint8_t* nonces = node->getBufferNonces();
        if (nvsStorage.saveNonces(nonces, RADIOLIB_LORAWAN_NONCES_BUF_SIZE)) {
            #if DEBUG_SERIAL
            Serial.println(F("[LoRa] DevNonce saved to NVS"));
            #endif
        }
        
        // RADIOLIB_LORAWAN_NEW_SESSION (-1118) means join was successful!
        if (result == RADIOLIB_ERR_NONE || result == RADIOLIB_LORAWAN_NEW_SESSION) {
            state = LORA_JOINED;
            uplinkCount = 0;
            
            #if DEBUG_SERIAL
            Serial.println(F("[LoRa] ✓ Join successful!"));
            Serial.print(F("[LoRa] Result code: "));
            Serial.println(result);
            #endif
            
            return true;
        }
        
        #if DEBUG_SERIAL
        Serial.print(F("[LoRa] Join attempt result: "));
        Serial.println(result);
        #endif
        
        // Wait before retry
        if (retry < maxRetries - 1) {
            delay(JOIN_RETRY_INTERVAL);
        }
    }
    
    state = LORA_JOIN_FAILED;
    
    #if DEBUG_SERIAL
    Serial.println(F("[LoRa] ✗ Join failed after all retries"));
    #endif
    
    return false;
}

bool LoRaWANModule::sendUplink(uint8_t* data, uint8_t len, uint8_t port, bool confirmed) {
    if (state != LORA_JOINED) {
        #if DEBUG_SERIAL
        Serial.println(F("[LoRa] Cannot send: not joined"));
        #endif
        return false;
    }
    
    // Ensure radio is ready - standby mode and RF switch configured
    radio->standby();
    radio->setDio2AsRfSwitch(true);
    
    // Check if we need to wait for duty cycle
    RadioLibTime_t waitTime = node->timeUntilUplink();
    #if DEBUG_SERIAL
    Serial.print(F("[LoRa] Time until uplink allowed: "));
    Serial.print(waitTime);
    Serial.println(F(" ms"));
    #endif
    
    if (waitTime > 0) {
        #if DEBUG_SERIAL
        Serial.print(F("[LoRa] Waiting for duty cycle..."));
        Serial.flush();
        #endif
        delay(waitTime + 100);  // Add small buffer
        #if DEBUG_SERIAL
        Serial.println(F(" done"));
        #endif
    }
    
    #if DEBUG_SERIAL
    Serial.print(F("[LoRa] Sending uplink ("));
    Serial.print(len);
    Serial.print(F(" bytes) on port "));
    Serial.print(port);
    Serial.print(F(", confirmed: "));
    Serial.println(confirmed ? F("YES") : F("NO"));
    
    Serial.print(F("[LoRa] Payload: "));
    for (uint8_t i = 0; i < len; i++) {
        if (data[i] < 0x10) Serial.print('0');
        Serial.print(data[i], HEX);
        Serial.print(' ');
    }
    Serial.println();
    #endif
    
    // Send uplink using sendReceive (handles MAC layer properly)
    // The third parameter (port) is where data is sent
    // Fourth parameter is downlink buffer (nullptr = no downlink expected for unconfirmed)
    int16_t result = node->sendReceive(data, len, port);
    
    // Handle specific errors/success codes
    // RADIOLIB_ERR_NONE = success with downlink
    // RADIOLIB_LORAWAN_NO_DOWNLINK = success, no downlink (normal for unconfirmed)
    // RADIOLIB_ERR_CRC_MISMATCH (-7) = RX got corrupted data (uplink likely succeeded)
    // RADIOLIB_ERR_RX_TIMEOUT (-6) = no downlink received (uplink likely succeeded)
    if (result == RADIOLIB_ERR_NONE || 
        result == RADIOLIB_LORAWAN_NO_DOWNLINK ||
        result == RADIOLIB_ERR_CRC_MISMATCH ||
        result == RADIOLIB_ERR_RX_TIMEOUT) {
        uplinkCount++;
        
        // Get RSSI and SNR
        lastRSSI = radio->getRSSI();
        lastSNR = radio->getSNR();
        
        #if DEBUG_SERIAL
        Serial.print(F("[LoRa] ✓ Uplink #"));
        Serial.print(uplinkCount);
        Serial.print(F(" sent (result: "));
        Serial.print(result);
        Serial.print(F("), RSSI: "));
        Serial.print(lastRSSI);
        Serial.print(F(" dBm, SNR: "));
        Serial.print(lastSNR);
        Serial.println(F(" dB"));
        #endif
        
        return true;
    } else {
        #if DEBUG_SERIAL
        Serial.print(F("[LoRa] ✗ Uplink failed, code: "));
        Serial.println(result);
        #endif
        return false;
    }
}

void LoRaWANModule::sleep() {
    radio->sleep();
    
    #if DEBUG_SERIAL
    Serial.println(F("[LoRa] Sleep mode"));
    #endif
}

void LoRaWANModule::wakeup() {
    radio->standby();
    
    #if DEBUG_SERIAL
    Serial.println(F("[LoRa] Wakeup"));
    #endif
}
