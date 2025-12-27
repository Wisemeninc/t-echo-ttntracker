#ifndef NVS_H
#define NVS_H

#include <Arduino.h>
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>

// LoRaWAN nonces buffer size from RadioLib
#define LORAWAN_NONCES_SIZE  16
#define LORAWAN_SESSION_SIZE 256

class NVSStorage {
public:
    NVSStorage();
    
    // Initialize the file system
    bool begin();
    
    // Save/load LoRaWAN nonces (DevNonce, JoinNonce, etc.)
    bool saveNonces(const uint8_t* buffer, size_t size);
    bool loadNonces(uint8_t* buffer, size_t size);
    bool hasNonces();
    
    // Save/load LoRaWAN session (for session restore without rejoin)
    bool saveSession(const uint8_t* buffer, size_t size);
    bool loadSession(uint8_t* buffer, size_t size);
    bool hasSession();
    
    // Clear all stored data (force rejoin)
    void clearAll();
    
private:
    bool initialized;
    
    static constexpr const char* NONCES_FILE = "/lorawan_nonces";
    static constexpr const char* SESSION_FILE = "/lorawan_session";
};

extern NVSStorage nvsStorage;

#endif // NVS_H
