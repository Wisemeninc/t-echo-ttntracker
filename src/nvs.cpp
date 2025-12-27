#include "nvs.h"
#include "../include/config.h"

using namespace Adafruit_LittleFS_Namespace;

NVSStorage nvsStorage;

NVSStorage::NVSStorage() : initialized(false) {
}

bool NVSStorage::begin() {
    // Initialize Internal File System
    if (!InternalFS.begin()) {
        #if DEBUG_SERIAL
        Serial.println(F("[NVS] Failed to initialize InternalFS"));
        #endif
        return false;
    }
    
    initialized = true;
    
    #if DEBUG_SERIAL
    Serial.println(F("[NVS] InternalFS initialized"));
    #endif
    
    return true;
}

bool NVSStorage::saveNonces(const uint8_t* buffer, size_t size) {
    if (!initialized) return false;
    
    File file(InternalFS);
    if (!file.open(NONCES_FILE, FILE_O_WRITE)) {
        #if DEBUG_SERIAL
        Serial.println(F("[NVS] Failed to open nonces file for writing"));
        #endif
        return false;
    }
    
    // Truncate and write
    file.seek(0);
    size_t written = file.write(buffer, size);
    file.close();
    
    #if DEBUG_SERIAL
    Serial.print(F("[NVS] Saved "));
    Serial.print(written);
    Serial.println(F(" bytes of nonces"));
    #endif
    
    return written == size;
}

bool NVSStorage::loadNonces(uint8_t* buffer, size_t size) {
    if (!initialized) return false;
    
    File file(InternalFS);
    if (!file.open(NONCES_FILE, FILE_O_READ)) {
        #if DEBUG_SERIAL
        Serial.println(F("[NVS] No nonces file found"));
        #endif
        return false;
    }
    
    size_t read = file.read(buffer, size);
    file.close();
    
    #if DEBUG_SERIAL
    Serial.print(F("[NVS] Loaded "));
    Serial.print(read);
    Serial.println(F(" bytes of nonces"));
    #endif
    
    return read == size;
}

bool NVSStorage::hasNonces() {
    if (!initialized) return false;
    
    File file(InternalFS);
    bool exists = file.open(NONCES_FILE, FILE_O_READ);
    if (exists) {
        file.close();
    }
    return exists;
}

bool NVSStorage::saveSession(const uint8_t* buffer, size_t size) {
    if (!initialized) return false;
    
    File file(InternalFS);
    if (!file.open(SESSION_FILE, FILE_O_WRITE)) {
        #if DEBUG_SERIAL
        Serial.println(F("[NVS] Failed to open session file for writing"));
        #endif
        return false;
    }
    
    file.seek(0);
    size_t written = file.write(buffer, size);
    file.close();
    
    #if DEBUG_SERIAL
    Serial.print(F("[NVS] Saved "));
    Serial.print(written);
    Serial.println(F(" bytes of session"));
    #endif
    
    return written == size;
}

bool NVSStorage::loadSession(uint8_t* buffer, size_t size) {
    if (!initialized) return false;
    
    File file(InternalFS);
    if (!file.open(SESSION_FILE, FILE_O_READ)) {
        #if DEBUG_SERIAL
        Serial.println(F("[NVS] No session file found"));
        #endif
        return false;
    }
    
    size_t read = file.read(buffer, size);
    file.close();
    
    #if DEBUG_SERIAL
    Serial.print(F("[NVS] Loaded "));
    Serial.print(read);
    Serial.println(F(" bytes of session"));
    #endif
    
    return read == size;
}

bool NVSStorage::hasSession() {
    if (!initialized) return false;
    
    File file(InternalFS);
    bool exists = file.open(SESSION_FILE, FILE_O_READ);
    if (exists) {
        file.close();
    }
    return exists;
}

void NVSStorage::clearAll() {
    if (!initialized) return;
    
    InternalFS.remove(NONCES_FILE);
    InternalFS.remove(SESSION_FILE);
    
    #if DEBUG_SERIAL
    Serial.println(F("[NVS] Cleared all stored data"));
    #endif
}
