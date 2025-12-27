#ifndef GPS_H
#define GPS_H

#include <Arduino.h>
#include <TinyGPSPlus.h>

// GPS data structure
struct GPSData {
    double latitude;
    double longitude;
    double altitude;
    double hdop;
    uint8_t satellites;
    bool valid;
    uint32_t fixAge;
};

class GPS {
public:
    GPS();
    
    // Initialization
    bool begin();
    
    // Power management
    void enable();
    void disable();
    void sleep();
    void wakeup();
    
    // GPS operations
    bool waitForFix(uint32_t timeout_ms);
    bool update();
    GPSData getData();
    
    // Status
    bool hasValidFix();
    uint8_t getSatellites();
    double getHDOP();
    
    // Direct access to TinyGPS++ object
    TinyGPSPlus& getGPS() { return gps; }

private:
    TinyGPSPlus gps;
    HardwareSerial* gpsSerial;
    bool isEnabled;
    
    void sendCommand(const char* cmd);
    void configureGPS();
};

// Global GPS instance
extern GPS gpsModule;

#endif // GPS_H
