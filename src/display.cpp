#include "display.h"
#include "../include/pins.h"
#include "../include/config.h"
#include <Fonts/FreeMonoBold9pt7b.h>

Display display;

// E-paper uses its own SPI bus (NRF_SPIM2) - separate from LoRa!
static SPIClass* dispPort = nullptr;

Display::Display() : isInitialized(false), epd(nullptr) {
}

bool Display::begin() {
    #if !DISPLAY_ENABLED
    return false;
    #endif
    
    #if DEBUG_SERIAL
    Serial.println(F("[Display] Initializing e-paper display..."));
    #endif
    
    // Create SPI port for display (NRF_SPIM2) - MUST be separate from LoRa (NRF_SPIM3)
    dispPort = new SPIClass(NRF_SPIM2, EPD_MISO, EPD_SCLK, EPD_MOSI);
    
    // Create GxEPD2 display instance
    // Constructor: (CS, DC, RST, BUSY)
    epd = new GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT>(
        GxEPD2_154_D67(EPD_CS, EPD_DC, EPD_RESET, EPD_BUSY)
    );
    
    // Select our SPI port and set SPI settings (4MHz, MSB first, Mode 0)
    epd->epd2.selectSPI(*dispPort, SPISettings(4000000, MSBFIRST, SPI_MODE0));
    
    // Initialize display
    dispPort->begin();
    epd->init(0);  // 0 = no serial debug
    
    epd->setRotation(DISPLAY_ROTATION);
    epd->setTextColor(GxEPD_BLACK);
    epd->setTextWrap(true);
    epd->setFullWindow();
    
    isInitialized = true;
    
    #if DEBUG_SERIAL
    Serial.println(F("[Display] E-paper initialized"));
    #endif
    
    return true;
}

void Display::clear() {
    if (!isInitialized) return;
    epd->fillScreen(GxEPD_WHITE);
}

void Display::sleep() {
    if (!isInitialized) return;
    epd->hibernate();
}

void Display::drawHeader() {
    epd->setFont(nullptr);  // Use built-in font
    epd->setTextSize(1);
    epd->setCursor(0, 5);
    epd->print(F("TTNMapper Tracker"));
    epd->drawLine(0, 12, 200, 12, GxEPD_BLACK);
    
    // Draw battery indicator
    float voltage = getBatteryVoltage();
    drawBattery(voltage);
}

void Display::drawBattery(float voltage) {
    // Battery icon in top right corner
    int x = 165;
    int y = 2;
    
    // Draw battery outline
    epd->drawRect(x, y, 30, 8, GxEPD_BLACK);
    epd->fillRect(x + 30, y + 2, 2, 4, GxEPD_BLACK);
    
    // Calculate fill level (3.0V = empty, 4.2V = full)
    int fillWidth = map(constrain(voltage * 100, 300, 420), 300, 420, 0, 28);
    if (fillWidth > 0) {
        epd->fillRect(x + 1, y + 1, fillWidth, 6, GxEPD_BLACK);
    }
    
    // Show voltage
    epd->setTextSize(1);
    epd->setCursor(x - 30, y + 1);
    epd->print(voltage, 1);
    epd->print(F("V"));
}

float Display::getBatteryVoltage() {
    // Read battery voltage from ADC
    // T-Echo has voltage divider: VBAT -> 1M -> ADC -> 1M -> GND
    analogReadResolution(12);  // 12-bit ADC
    int raw = analogRead(VBAT_PIN);
    
    // Convert to voltage: (raw / 4096) * 3.6V * 2 (voltage divider)
    float voltage = (raw / 4096.0) * 3.6 * 2.0;
    return voltage;
}

void Display::showStartup() {
    if (!isInitialized) return;
    
    epd->setFullWindow();
    epd->firstPage();
    do {
        epd->fillScreen(GxEPD_WHITE);
        drawHeader();
        
        epd->setTextSize(2);
        epd->setCursor(20, 70);
        epd->print(F("Starting..."));
        
        epd->setTextSize(1);
        epd->setCursor(10, 110);
        epd->print(F("Lilygo T-Echo"));
        epd->setCursor(10, 125);
        epd->print(F("GPS Tracker"));
    } while (epd->nextPage());
}

void Display::showJoining(uint8_t attempt, uint8_t maxAttempts) {
    if (!isInitialized) return;
    
    epd->setFullWindow();
    epd->firstPage();
    do {
        epd->fillScreen(GxEPD_WHITE);
        drawHeader();
        
        epd->setTextSize(2);
        epd->setCursor(10, 50);
        epd->print(F("Joining..."));
        
        epd->setTextSize(1);
        epd->setCursor(10, 80);
        epd->print(F("TTN Network"));
        
        epd->setCursor(10, 100);
        epd->print(F("Attempt: "));
        epd->print(attempt);
        epd->print(F("/"));
        epd->print(maxAttempts);
    } while (epd->nextPage());
}

void Display::showJoined() {
    if (!isInitialized) return;
    
    #if DEBUG_SERIAL
    Serial.println(F("[Display] showJoined() start"));
    Serial.flush();
    #endif
    
    epd->setFullWindow();
    epd->firstPage();
    do {
        epd->fillScreen(GxEPD_WHITE);
        drawHeader();
        
        epd->setTextSize(2);
        epd->setCursor(30, 70);
        epd->print(F("JOINED!"));
        
        epd->setTextSize(1);
        epd->setCursor(10, 110);
        epd->print(F("Ready to track"));
    } while (epd->nextPage());
    
    #if DEBUG_SERIAL
    Serial.println(F("[Display] showJoined() done"));
    Serial.flush();
    #endif
}

void Display::showJoinFailed() {
    if (!isInitialized) return;
    
    epd->setFullWindow();
    epd->firstPage();
    do {
        epd->fillScreen(GxEPD_WHITE);
        drawHeader();
        
        epd->setTextSize(2);
        epd->setCursor(10, 70);
        epd->print(F("Join Fail"));
        
        epd->setTextSize(1);
        epd->setCursor(10, 110);
        epd->print(F("Check credentials"));
        epd->setCursor(10, 125);
        epd->print(F("Restarting..."));
    } while (epd->nextPage());
}

void Display::showGPSSearching() {
    if (!isInitialized) return;
    
    epd->setFullWindow();
    epd->firstPage();
    do {
        epd->fillScreen(GxEPD_WHITE);
        drawHeader();
        
        epd->setTextSize(2);
        epd->setCursor(10, 50);
        epd->print(F("GPS Fix..."));
        
        epd->setTextSize(1);
        epd->setCursor(10, 80);
        epd->print(F("Searching satellites"));
    } while (epd->nextPage());
}

void Display::showGPSFix(GPSData data) {
    if (!isInitialized) return;
    
    epd->setFullWindow();
    epd->firstPage();
    do {
        epd->fillScreen(GxEPD_WHITE);
        drawHeader();
        
        epd->setTextSize(1);
        epd->setCursor(5, 25);
        epd->print(F("GPS Fix Acquired!"));
        
        epd->setCursor(5, 50);
        epd->print(F("Lat: "));
        epd->print(data.latitude, 6);
        
        epd->setCursor(5, 65);
        epd->print(F("Lon: "));
        epd->print(data.longitude, 6);
        
        epd->setCursor(5, 80);
        epd->print(F("Alt: "));
        epd->print(data.altitude, 1);
        epd->print(F("m"));
        
        epd->setCursor(5, 95);
        epd->print(F("Sats: "));
        epd->print(data.satellites);
        epd->print(F("  HDOP: "));
        epd->print(data.hdop, 1);
    } while (epd->nextPage());
}

void Display::showTransmitting(uint32_t count) {
    if (!isInitialized) return;
    
    epd->setFullWindow();
    epd->firstPage();
    do {
        epd->fillScreen(GxEPD_WHITE);
        drawHeader();
        
        epd->setTextSize(2);
        epd->setCursor(10, 70);
        epd->print(F("TX #"));
        epd->print(count);
    } while (epd->nextPage());
}

void Display::showStatus(GPSData gpsData, LoRaWANState loraState, uint32_t txCount) {
    if (!isInitialized) return;
    
    epd->setFullWindow();
    epd->firstPage();
    do {
        epd->fillScreen(GxEPD_WHITE);
        drawHeader();
        
        // LoRa status
        epd->setTextSize(1);
        epd->setCursor(5, 25);
        epd->print(F("LoRa: "));
        switch (loraState) {
            case LORA_JOINED:
                epd->print(F("Joined"));
                break;
            case LORA_JOINING:
                epd->print(F("Joining..."));
                break;
            case LORA_JOIN_FAILED:
                epd->print(F("Join Failed"));
                break;
            default:
                epd->print(F("Not Joined"));
                break;
        }
        
        epd->setCursor(5, 40);
        epd->print(F("TX Count: "));
        epd->print(txCount);
        
        // GPS status
        epd->setCursor(5, 60);
        epd->print(F("GPS: "));
        if (gpsData.valid) {
            epd->print(F("Fix OK"));
        } else {
            epd->print(F("Searching..."));
        }
        
        if (gpsData.valid) {
            epd->setCursor(5, 80);
            epd->print(F("Lat: "));
            epd->print(gpsData.latitude, 4);
            
            epd->setCursor(5, 95);
            epd->print(F("Lon: "));
            epd->print(gpsData.longitude, 4);
            
            epd->setCursor(5, 110);
            epd->print(F("Sats: "));
            epd->print(gpsData.satellites);
            epd->print(F(" HDOP: "));
            epd->print(gpsData.hdop, 1);
        }
        
        // Next update
        epd->setCursor(5, 130);
        epd->print(F("Next TX in 60s"));
    } while (epd->nextPage());
}

void Display::showError(const char* message) {
    if (!isInitialized) return;
    
    epd->setFullWindow();
    epd->firstPage();
    do {
        epd->fillScreen(GxEPD_WHITE);
        drawHeader();
        
        epd->setTextSize(2);
        epd->setCursor(30, 50);
        epd->print(F("ERROR"));
        
        epd->setTextSize(1);
        epd->setCursor(5, 90);
        epd->print(message);
    } while (epd->nextPage());
}
