# T-Echo TTNMapper GPS Tracker

A complete firmware solution for the **Lilygo T-Echo** development board that sends GPS location data to **TTNMapper.org** via **The Things Network (TTN)** LoRaWAN network. The tracker transmits location data every 3 minutes with visual feedback on the integrated e-paper display.

## Features

✅ **LoRaWAN OTAA** - Over-The-Air Activation with TTN (EU868)  
✅ **GPS Tracking** - L76K GPS module with GPS+GLONASS support  
✅ **TTNMapper Integration** - Optimized 9-byte binary payload  
✅ **E-Paper Display** - Real-time status on 1.54" display  
✅ **Power Management** - Sleep mode between transmissions  
✅ **Battery Monitoring** - Voltage display on screen  
✅ **LED Indicators** - Visual feedback for operations  

## Hardware Requirements

- **Lilygo T-Echo** development board
  - nRF52840 MCU
  - SX1262 LoRa radio (868 MHz)
  - L76K GPS module
  - 1.54" e-paper display
  - Built-in battery management

## Software Requirements

- [PlatformIO](https://platformio.org/) (recommended) or Arduino IDE
- [The Things Network](https://www.thethingsnetwork.org/) account
- LoRaWAN gateway coverage (check [TTNMapper.org](https://ttnmapper.org/))

## Quick Start

### 1. Clone the Repository

```bash
git clone <repository-url>
cd t-echo-ttntracker
```

### 2. Configure TTN Credentials

Edit `include/config.h` and replace the placeholder values with your TTN device credentials:

```cpp
// JoinEUI (AppEUI) - from TTN Console
uint64_t joinEUI = 0x0000000000000000;

// DevEUI - unique device identifier
uint64_t devEUI = 0x70B3D57ED005XXXX;

// AppKey - 16 bytes from TTN Console
uint8_t appKey[] = {
    0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX,
    0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX
};

// NwkKey - MUST be set to the same value as AppKey for LoRaWAN 1.0.x
uint8_t nwkKey[] = {
    0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX,
    0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX
};
```

> ⚠️ **Important:** For LoRaWAN 1.0.x (used by TTN), the `nwkKey` must be identical to `appKey`. RadioLib requires both keys, and setting them to the same value ensures proper MIC (Message Integrity Code) calculation during OTAA join.

### 3. Build and Upload

**Using PlatformIO:**
```bash
pio run --target upload
```

**Using PlatformIO IDE:**
- Open the project folder in VS Code with PlatformIO extension
- Click "Upload" button in the PlatformIO toolbar

### 4. Configure TTN Application

#### Create Application & Device

1. Go to [TTN Console](https://console.cloud.thethings.network/)
2. Create a new application
3. Register a new end device:
   - **LoRaWAN version:** LoRaWAN 1.0.3
   - **Regional Parameters:** RP001 Regional Parameters 1.0.3 revision A
   - **Frequency plan:** Europe 863-870 MHz (SF9 for RX2)
   - **Activation mode:** OTAA
   - Generate DevEUI and AppKey (or use custom values)

#### Add Payload Decoder

1. In your application, go to **Payload formatters** → **Uplink**
2. Select **Custom Javascript formatter**
3. Copy the contents of `ttn-decoder.js` and paste it
4. Save changes

#### Add TTNMapper Integration

1. Go to **Integrations** → **Webhooks**
2. Add webhook:
   - **Webhook ID:** `ttnmapper`
   - **Webhook format:** JSON
   - **Base URL:** `https://integrations.ttnmapper.org/tts/v3`
   - **Enabled:** ✓
3. Save webhook

**Important:** Also add your email address in the TTNMapper experiment settings (optional but recommended for data attribution).

### 5. Monitor Operation

**Serial Monitor (115200 baud):**
```
========================================
  T-Echo TTNMapper GPS Tracker
========================================
[Init] Initializing hardware...
[GPS] Initialized L76K GPS module
[LoRa] SX1262 initialized successfully
[State] JOINING TTN...
[LoRa] ✓ Join successful!
[GPS] Valid fix acquired!
[LoRa] ✓ Uplink #1 successful
```

**E-Paper Display shows:**
- Battery voltage
- GPS status (satellites, HDOP)
- LoRa status (joined/not joined)
- Transmission count
- Current coordinates

## Configuration Options

Edit `include/config.h` to customize:

| Setting | Default | Description |
|---------|---------|-------------|
| `TX_INTERVAL_MS` | 180000 (3 min) | Time between transmissions |
| `GPS_FIX_TIMEOUT_MS` | 60000 (60s) | Max time to wait for GPS fix |
| `MIN_SATELLITES` | 4 | Minimum satellites for valid fix |
| `LORAWAN_DATARATE` | 5 (SF7) | LoRaWAN spreading factor |
| `LORAWAN_TX_POWER` | 14 dBm | Transmission power (max for EU868) |
| `LORAWAN_CONFIRMED` | false | Use confirmed uplinks |
| `DEBUG_SERIAL` | true | Enable serial debug output |
| `DISPLAY_ENABLED` | true | Enable e-paper display updates |
| `DISPLAY_ROTATION` | 3 | Display rotation (0-3, 90° increments) |

## Payload Format

The tracker sends a **9-byte binary payload** optimized for TTNMapper:

| Bytes | Field | Encoding |
|-------|-------|----------|
| 0-2 | Latitude | 3 bytes: `((lat + 90) / 180) × 16777215` |
| 3-5 | Longitude | 3 bytes: `((lon + 180) / 360) × 16777215` |
| 6-7 | Altitude | 2 bytes: signed int16 (meters) |
| 8 | HDOP | 1 byte: `HDOP × 10` |

**Decoded JSON (after TTN decoder):**
```json
{
  "latitude": 52.520008,
  "longitude": 13.404954,
  "altitude": 50,
  "hdop": 1.2
}
```

## LED Indicators

| LED | Pattern | Meaning |
|-----|---------|---------|
| Green | 3 quick blinks | Startup |
| Blue | Solid (brief) | Transmitting |
| Green | 2 blinks | Transmission successful |
| Red | 3 slow blinks | Transmission failed |
| Red | Continuous | Fatal error |

## Troubleshooting

### GPS Issues

**Problem:** No GPS fix after 60 seconds  
**Solutions:**
- Ensure clear view of sky (outdoors)
- Check GPS antenna connection
- Wait longer on first boot (cold start can take 2-5 minutes)
- Verify GPS power enable on pin 12

**Problem:** GPS reports 0 satellites  
**Solutions:**
- Check serial connection (P1.8 TX, P1.9 RX)
- Verify 9600 baud rate
- Reset GPS module (toggle GPS_RESET_PIN)

### LoRaWAN Issues

**Problem:** Join failed after all retries  
**Solutions:**
- Verify credentials in `config.h` (MSB format)
- Check LoRaWAN gateway coverage
- Ensure antenna is connected
- Check frequency plan matches region (EU868)
- Verify DevEUI is unique

**Problem:** Uplink successful but no data on TTN  
**Solutions:**
- Check TTN Console live data view
- Verify payload decoder is configured
- Check application is not suspended

### Display Issues

**Problem:** E-paper display shows nothing  
**Solutions:**
- Try power cycling the device
- Set `DISPLAY_ENABLED` to `false` to disable
- E-paper refresh takes 2-5 seconds (this is normal)

## Hardware Notes

### SPI Bus Separation

The T-Echo uses **two separate SPI buses** to avoid conflicts:

| Peripheral | SPI Instance | SCK | MOSI | MISO | CS |
|------------|--------------|-----|------|------|-----|
| E-Paper Display | NRF_SPIM2 | P0.31 | P0.29 | P0.04 | P0.30 |
| SX1262 LoRa | NRF_SPIM3 | P0.19 | P0.22 | P0.23 | P0.24 |

### SX1262 Configuration

The SX1262 radio requires specific configuration for the T-Echo:

- **TCXO Voltage:** 1.6V (must be set or radio won't initialize)
- **DIO2 as RF Switch:** Enabled (controls TX/RX antenna switch)
- **DIO3 as TCXO Control:** Enabled
- **RX Boosted Gain:** Enabled for better sensitivity

### GPS Module

The L76K/AT6558R GPS module configuration:

- **UART:** Serial1 on P1.08 (TX) / P1.09 (RX)
- **Baud Rate:** 9600
- **Constellations:** GPS + GLONASS
- **Reset Pin:** P1.05 (active low)
- **PPS Pin:** P1.04

## Power Consumption

| Mode | Current | Duration |
|------|---------|----------|
| GPS Fix | ~40 mA | 10-60s |
| LoRa TX | ~120 mA | 1-2s |
| Display Update | ~15 mA | 2-3s |
| Sleep | ~5 mA | 3 min |

**Estimated battery life (1000 mAh):**
- Continuous operation (3 min interval): ~24-36 hours
- With deep sleep optimization: ~5-7 days

## TTNMapper Coverage Mapping

This tracker is designed for **TTNMapper.org** coverage mapping:

1. **Mobility Required** - Move the tracker to different locations
2. **Valid GPS Fix** - Requires 4+ satellites and HDOP < 25.5
3. **Gateway Coverage** - Must have LoRaWAN gateway in range
4. **Data Upload** - TTNMapper webhook automatically processes uplinks

**View your contributions:**
- Go to [ttnmapper.org](https://ttnmapper.org/)
- Search for your device DevEUI or application name

## Development

### Project Structure

```
t-echo-ttntracker/
├── platformio.ini          # PlatformIO configuration
├── include/
│   ├── config.h            # User configuration & credentials
│   └── pins.h              # T-Echo hardware pin definitions
├── src/
│   ├── main.cpp            # Main application loop
│   ├── gps.cpp/h           # GPS module (L76K)
│   ├── lora.cpp/h          # LoRaWAN module (SX1262)
│   ├── payload.cpp/h       # TTNMapper payload encoder
│   └── display.cpp/h       # E-paper display driver
├── ttn-decoder.js          # TTN payload decoder (JavaScript)
└── README.md               # This file
```

### Dependencies

- **RadioLib** v6.6.0+ - LoRaWAN stack for SX1262
- **TinyGPSPlus** v1.0.3+ - GPS NMEA parsing
- **Adafruit GFX Library** v1.11.9+ - Graphics primitives
- **GxEPD2** v1.6.0+ - E-paper display driver (SSD1681)
- **Adafruit SPIFlash** v4.0.0+ - Flash memory access
- **Adafruit BusIO** v1.16.1+ - I2C/SPI abstraction

### Customization

**Change transmission interval:**
```cpp
#define TX_INTERVAL_MS (5 * 60 * 1000)  // 5 minutes
```

**Change LoRaWAN region:**
Edit `platformio.ini`:
```ini
build_flags = 
    -DREGION_US915  # For US915
```

**Disable display to save power:**
```cpp
#define DISPLAY_ENABLED false
```

## Fair Use Policy

**The Things Network Fair Use Policy:**
- Maximum **30 seconds** of airtime per device per day
- SF7BW125 at 14dBm ≈ **50-60ms** per uplink
- 9-byte payload with overhead ≈ **60ms**
- 3-minute interval = **480 uplinks/day** = **~29 seconds** ✓

**This configuration respects TTN Fair Use Policy.**

## License

This project is released under the MIT License. See LICENSE file for details.

## Credits

- **Hardware:** [Lilygo T-Echo](https://www.lilygo.cc/)
- **LoRaWAN Stack:** [RadioLib](https://github.com/jgromes/RadioLib)
- **GPS Library:** [TinyGPSPlus](https://github.com/mikalhart/TinyGPSPlus)
- **Network:** [The Things Network](https://www.thethingsnetwork.org/)
- **Mapping:** [TTNMapper.org](https://ttnmapper.org/)

## Support

- **Issues:** Open an issue on GitHub
- **TTN Forum:** [The Things Network Forum](https://www.thethingsnetwork.org/forum/)
- **TTNMapper:** [TTNMapper Discourse](https://discourse.ttnmapper.org/)

## Version History

**v1.0.1** (2025-12-24)
- Fixed LoRaWAN 1.0.x join by setting nwkKey = appKey
- Switched to GxEPD2 library for proper display support
- Fixed SPI bus separation (SPIM2 for display, SPIM3 for LoRa)
- Fixed uplink errors by using sendReceive() for proper MAC handling
- Added display rotation support
- Improved GPS fix handling

**v1.0.0** (2025-12-23)
- Initial release
- EU868 support
- 3-minute transmission interval
- E-paper display integration
- TTNMapper binary payload format
- GPS+GLONASS support
- Battery monitoring

---

**Happy Mapping! 🗺️📡**
