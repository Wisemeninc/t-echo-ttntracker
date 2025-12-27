#ifndef LORA_H
#define LORA_H

#include <Arduino.h>
#include <RadioLib.h>

// LoRaWAN session state
enum LoRaWANState {
    LORA_NOT_JOINED,
    LORA_JOINING,
    LORA_JOINED,
    LORA_JOIN_FAILED
};

class LoRaWANModule {
public:
    LoRaWANModule();
    
    // Initialization
    bool begin();
    
    // OTAA Join
    bool join(uint8_t maxRetries = 10);
    bool isJoined() { return state == LORA_JOINED; }
    LoRaWANState getState() { return state; }
    
    // Transmission
    bool sendUplink(uint8_t* data, uint8_t len, uint8_t port = 1, bool confirmed = false);
    
    // Power management
    void sleep();
    void wakeup();
    
    // Statistics
    uint32_t getUplinkCount() { return uplinkCount; }
    int16_t getLastRSSI() { return lastRSSI; }
    int8_t getLastSNR() { return lastSNR; }
    
    // Direct access to LoRaWAN node
    LoRaWANNode* getNode() { return node; }

private:
    SPIClass* rfPort;
    Module* radioModule;
    SX1262* radio;
    LoRaWANNode* node;
    LoRaWANState state;
    
    uint32_t uplinkCount;
    int16_t lastRSSI;
    int8_t lastSNR;
    
    bool configureRadio();
};

// Global LoRaWAN instance
extern LoRaWANModule loraModule;

#endif // LORA_H
