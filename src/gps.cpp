#include "gps.h"
#include "../include/pins.h"
#include "../include/config.h"

GPS gpsModule;

GPS::GPS() : isEnabled(false) {
    gpsSerial = &Serial1;  // Use Serial1 for GPS on nRF52840
}

bool GPS::begin() {
    // Enable power to GPS module
    pinMode(PIN_POWER_EN, OUTPUT);
    digitalWrite(PIN_POWER_EN, HIGH);
    delay(100);
    
    // Configure GPS control pins
    pinMode(GPS_WAKEUP_PIN, OUTPUT);
    pinMode(GPS_RESET_PIN, OUTPUT);
    
    // Reset GPS module first
    digitalWrite(GPS_RESET_PIN, LOW);
    delay(100);
    digitalWrite(GPS_RESET_PIN, HIGH);
    delay(100);
    
    // Keep GPS awake
    digitalWrite(GPS_WAKEUP_PIN, HIGH);
    
    // Initialize UART for GPS
    // Serial1 pins are defined in variant.h: RX=P1.9, TX=P1.8
    gpsSerial->begin(GPS_BAUD_RATE);
    
    delay(1000);  // Give GPS time to boot
    
    #if DEBUG_SERIAL
    Serial.print(F("[GPS] UART initialized at "));
    Serial.print(GPS_BAUD_RATE);
    Serial.println(F(" baud"));
    Serial.print(F("[GPS] Checking for data..."));
    
    // Check if GPS is sending any data
    uint32_t checkStart = millis();
    int bytesFound = 0;
    while (millis() - checkStart < 2000) {
        if (gpsSerial->available()) {
            bytesFound++;
            char c = gpsSerial->read();
            if (bytesFound < 100) Serial.print(c);
        }
    }
    Serial.println();
    Serial.print(F("[GPS] Received "));
    Serial.print(bytesFound);
    Serial.println(F(" bytes in 2 seconds"));
    #endif
    
    // Configure GPS for optimal performance
    configureGPS();
    
    isEnabled = true;
    
    #if DEBUG_SERIAL
    Serial.println(F("[GPS] Initialized L76K GPS module"));
    #endif
    
    return true;
}

void GPS::enable() {
    if (!isEnabled) {
        digitalWrite(PIN_POWER_EN, HIGH);
        digitalWrite(GPS_WAKEUP_PIN, HIGH);
        delay(100);
        gpsSerial->begin(GPS_BAUD_RATE);
        isEnabled = true;
        
        #if DEBUG_SERIAL
        Serial.println(F("[GPS] Enabled"));
        #endif
    }
}

void GPS::disable() {
    if (isEnabled) {
        sleep();
        gpsSerial->end();
        digitalWrite(GPS_WAKEUP_PIN, LOW);
        isEnabled = false;
        
        #if DEBUG_SERIAL
        Serial.println(F("[GPS] Disabled"));
        #endif
    }
}

void GPS::sleep() {
    // Send standby command to L76K
    sendCommand("$PMTK161,0*28");  // Standby mode
    delay(100);
    digitalWrite(GPS_WAKEUP_PIN, LOW);
    
    #if DEBUG_SERIAL
    Serial.println(F("[GPS] Sleep mode"));
    #endif
}

void GPS::wakeup() {
    digitalWrite(GPS_WAKEUP_PIN, HIGH);
    delay(100);
    
    // Send any byte to wake up from standby
    gpsSerial->write(0xFF);
    delay(100);
    
    #if DEBUG_SERIAL
    Serial.println(F("[GPS] Wakeup"));
    #endif
}

void GPS::configureGPS() {
    // Set GPS+GLONASS mode for better accuracy
    // PMTK353: GPS + GLONASS
    sendCommand("$PMTK353,1,1,0,0,0*2A");
    delay(100);
    
    // Set update rate to 1Hz
    sendCommand("$PMTK220,1000*1F");
    delay(100);
    
    // Set NMEA output: GGA, RMC, GSA, GSV
    sendCommand("$PMTK314,0,1,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0*28");
    delay(100);
    
    #if DEBUG_SERIAL
    Serial.println(F("[GPS] Configuration: GPS+GLONASS, 1Hz, GGA+RMC+GSA+GSV"));
    #endif
}

void GPS::sendCommand(const char* cmd) {
    gpsSerial->println(cmd);
    gpsSerial->flush();
}

bool GPS::update() {
    while (gpsSerial->available() > 0) {
        char c = gpsSerial->read();
        gps.encode(c);
    }
    return hasValidFix();
}

bool GPS::waitForFix(uint32_t timeout_ms) {
    #if DEBUG_SERIAL
    Serial.print(F("[GPS] Waiting for fix (timeout: "));
    Serial.print(timeout_ms / 1000);
    Serial.println(F("s)..."));
    #endif
    
    uint32_t startTime = millis();
    uint32_t lastPrint = 0;
    uint32_t bytesReceived = 0;
    
    while (millis() - startTime < timeout_ms) {
        // Count and process bytes
        while (gpsSerial->available() > 0) {
            char c = gpsSerial->read();
            bytesReceived++;
            gps.encode(c);
            
            #if DEBUG_SERIAL
            // Print raw NMEA data for first 10 seconds
            if (millis() - startTime < 10000) {
                Serial.print(c);
            }
            #endif
        }
        
        #if DEBUG_SERIAL
        // Print status every 5 seconds
        if (millis() - lastPrint > 5000) {
            Serial.println();
            Serial.print(F("[GPS] Bytes: "));
            Serial.print(bytesReceived);
            Serial.print(F(", Sats: "));
            Serial.print(gps.satellites.value());
            Serial.print(F(", HDOP: "));
            Serial.print(gps.hdop.hdop());
            Serial.print(F(", Valid: "));
            Serial.println(hasValidFix() ? F("YES") : F("NO"));
            lastPrint = millis();
        }
        #endif
        
        if (hasValidFix()) {
            #if DEBUG_SERIAL
            Serial.println(F("[GPS] Valid fix acquired!"));
            Serial.print(F("[GPS] Location: "));
            Serial.print(gps.location.lat(), 6);
            Serial.print(F(", "));
            Serial.print(gps.location.lng(), 6);
            Serial.print(F(", Alt: "));
            Serial.print(gps.altitude.meters(), 1);
            Serial.println(F("m"));
            #endif
            return true;
        }
        
        delay(100);
    }
    
    #if DEBUG_SERIAL
    Serial.println(F("[GPS] Timeout - no valid fix"));
    #endif
    
    return false;
}

bool GPS::hasValidFix() {
    return gps.location.isValid() && 
           gps.location.age() < 2000 &&
           gps.satellites.value() >= MIN_SATELLITES &&
           gps.hdop.isValid();
}

uint8_t GPS::getSatellites() {
    return gps.satellites.value();
}

double GPS::getHDOP() {
    return gps.hdop.hdop();
}

GPSData GPS::getData() {
    GPSData data;
    
    data.valid = hasValidFix();
    
    if (data.valid) {
        data.latitude = gps.location.lat();
        data.longitude = gps.location.lng();
        data.altitude = gps.altitude.meters();
        data.hdop = gps.hdop.hdop();
        data.satellites = gps.satellites.value();
        data.fixAge = gps.location.age();
    } else {
        data.latitude = 0.0;
        data.longitude = 0.0;
        data.altitude = 0.0;
        data.hdop = 99.9;
        data.satellites = 0;
        data.fixAge = 0xFFFFFFFF;
    }
    
    return data;
}
