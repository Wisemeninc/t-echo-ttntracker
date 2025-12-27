#include "payload.h"
#include "../include/config.h"

PayloadEncoder payloadEncoder;

PayloadEncoder::PayloadEncoder() {
    memset(payloadBuffer, 0, TTNMAPPER_PAYLOAD_SIZE);
}

uint8_t PayloadEncoder::encode(GPSData gpsData, uint8_t* buffer) {
    if (!gpsData.valid) {
        #if DEBUG_SERIAL
        Serial.println(F("[Payload] Cannot encode: invalid GPS data"));
        #endif
        return 0;
    }
    
    // Encode latitude (3 bytes)
    uint32_t latEncoded = encodeLatitude(gpsData.latitude);
    buffer[0] = (latEncoded >> 16) & 0xFF;
    buffer[1] = (latEncoded >> 8) & 0xFF;
    buffer[2] = latEncoded & 0xFF;
    
    // Encode longitude (3 bytes)
    uint32_t lonEncoded = encodeLongitude(gpsData.longitude);
    buffer[3] = (lonEncoded >> 16) & 0xFF;
    buffer[4] = (lonEncoded >> 8) & 0xFF;
    buffer[5] = lonEncoded & 0xFF;
    
    // Encode altitude (2 bytes, signed)
    int16_t altEncoded = encodeAltitude(gpsData.altitude);
    buffer[6] = (altEncoded >> 8) & 0xFF;
    buffer[7] = altEncoded & 0xFF;
    
    // Encode HDOP (1 byte)
    buffer[8] = encodeHDOP(gpsData.hdop);
    
    #if DEBUG_SERIAL
    Serial.println(F("[Payload] Encoded GPS data:"));
    Serial.print(F("  Lat: "));
    Serial.print(gpsData.latitude, 6);
    Serial.print(F(" -> 0x"));
    Serial.println(latEncoded, HEX);
    
    Serial.print(F("  Lon: "));
    Serial.print(gpsData.longitude, 6);
    Serial.print(F(" -> 0x"));
    Serial.println(lonEncoded, HEX);
    
    Serial.print(F("  Alt: "));
    Serial.print(gpsData.altitude, 1);
    Serial.print(F("m -> "));
    Serial.println(altEncoded);
    
    Serial.print(F("  HDOP: "));
    Serial.print(gpsData.hdop, 1);
    Serial.print(F(" -> "));
    Serial.println(buffer[8]);
    #endif
    
    return TTNMAPPER_PAYLOAD_SIZE;
}

uint32_t PayloadEncoder::encodeLatitude(double lat) {
    // Encode latitude: ((lat + 90) / 180) * 16777215
    // Range: -90 to +90 degrees
    // Output: 0 to 16777215 (24-bit)
    
    double normalized = (lat + 90.0) / 180.0;  // Normalize to 0-1
    uint32_t encoded = (uint32_t)(normalized * 16777215.0);
    
    // Clamp to valid range
    if (encoded > 16777215) encoded = 16777215;
    
    return encoded;
}

uint32_t PayloadEncoder::encodeLongitude(double lon) {
    // Encode longitude: ((lon + 180) / 360) * 16777215
    // Range: -180 to +180 degrees
    // Output: 0 to 16777215 (24-bit)
    
    double normalized = (lon + 180.0) / 360.0;  // Normalize to 0-1
    uint32_t encoded = (uint32_t)(normalized * 16777215.0);
    
    // Clamp to valid range
    if (encoded > 16777215) encoded = 16777215;
    
    return encoded;
}

int16_t PayloadEncoder::encodeAltitude(double alt) {
    // Encode altitude as signed 16-bit integer (meters)
    // Range: -32768 to +32767 meters
    
    int16_t encoded = (int16_t)alt;
    
    // Clamp to valid range
    if (alt > 32767.0) encoded = 32767;
    if (alt < -32768.0) encoded = -32768;
    
    return encoded;
}

uint8_t PayloadEncoder::encodeHDOP(double hdop) {
    // Encode HDOP as unsigned 8-bit integer (HDOP * 10)
    // Range: 0.0 to 25.5 (stored as 0 to 255)
    
    uint8_t encoded = (uint8_t)(hdop * 10.0);
    
    // Clamp to valid range
    if (hdop > 25.5) encoded = 255;
    
    return encoded;
}
