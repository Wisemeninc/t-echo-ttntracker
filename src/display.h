#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <epd/GxEPD2_154_D67.h>
#include "gps.h"
#include "lora.h"

// Display type for T-Echo 1.54" e-paper (GDEH0154D67 / SSD1681)
#define GxEPD2_DISPLAY_CLASS GxEPD2_154_D67
#define GxEPD2_DRIVER_CLASS GxEPD2_154_D67
#define MAX_DISPLAY_BUFFER_SIZE 5000

class Display {
public:
    Display();
    
    // Initialization
    bool begin();
    
    // Display operations
    void showStartup();
    void showJoining(uint8_t attempt, uint8_t maxAttempts);
    void showJoined();
    void showJoinFailed();
    void showGPSSearching();
    void showGPSFix(GPSData data);
    void showTransmitting(uint32_t count);
    void showStatus(GPSData gpsData, LoRaWANState loraState, uint32_t txCount);
    void showError(const char* message);
    
    // Power management
    void sleep();
    void clear();

private:
    GxEPD2_BW<GxEPD2_DISPLAY_CLASS, GxEPD2_DISPLAY_CLASS::HEIGHT>* epd;
    bool isInitialized;
    
    void drawHeader();
    void drawBattery(float voltage);
    float getBatteryVoltage();
};

// Global display instance
extern Display display;

#endif // DISPLAY_H
