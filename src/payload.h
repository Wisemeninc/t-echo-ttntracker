#ifndef PAYLOAD_H
#define PAYLOAD_H

#include <Arduino.h>
#include "gps.h"

// TTNMapper payload format: 9 bytes
// Bytes 0-2: Latitude (3 bytes, encoded)
// Bytes 3-5: Longitude (3 bytes, encoded)
// Bytes 6-7: Altitude (2 bytes, signed int16)
// Byte 8:    HDOP (1 byte, HDOP * 10)

#define TTNMAPPER_PAYLOAD_SIZE 9

class PayloadEncoder {
public:
    PayloadEncoder();
    
    // Encode GPS data to TTNMapper binary format
    uint8_t encode(GPSData gpsData, uint8_t* buffer);
    
    // Helper functions
    static uint32_t encodeLatitude(double lat);
    static uint32_t encodeLongitude(double lon);
    static int16_t encodeAltitude(double alt);
    static uint8_t encodeHDOP(double hdop);
    
private:
    uint8_t payloadBuffer[TTNMAPPER_PAYLOAD_SIZE];
};

// Global payload encoder instance
extern PayloadEncoder payloadEncoder;

#endif // PAYLOAD_H
