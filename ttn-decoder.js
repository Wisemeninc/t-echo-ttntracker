// TTNMapper Payload Decoder for The Things Network
// Paste this into your TTN Application -> Payload Formatters -> Uplink

function decodeUplink(input) {
  var decoded = {};
  var bytes = input.bytes;
  
  // Check payload length
  if (bytes.length !== 9) {
    return {
      data: {},
      warnings: ["Invalid payload length: expected 9 bytes, got " + bytes.length],
      errors: []
    };
  }
  
  // Decode latitude (bytes 0-2)
  // Formula: ((value / 16777215) * 180) - 90
  var latRaw = (bytes[0] << 16) | (bytes[1] << 8) | bytes[2];
  decoded.latitude = ((latRaw / 16777215.0) * 180.0) - 90.0;
  
  // Decode longitude (bytes 3-5)
  // Formula: ((value / 16777215) * 360) - 180
  var lonRaw = (bytes[3] << 16) | (bytes[4] << 8) | bytes[5];
  decoded.longitude = ((lonRaw / 16777215.0) * 360.0) - 180.0;
  
  // Decode altitude (bytes 6-7) - signed int16
  var altRaw = (bytes[6] << 8) | bytes[7];
  // Handle negative values (two's complement)
  if (altRaw & 0x8000) {
    decoded.altitude = altRaw - 0x10000;
  } else {
    decoded.altitude = altRaw;
  }
  
  // Decode HDOP (byte 8)
  // Formula: value / 10
  decoded.hdop = bytes[8] / 10.0;
  
  return {
    data: decoded,
    warnings: [],
    errors: []
  };
}

// For TTN v2 compatibility (legacy)
function Decoder(bytes, port) {
  var decoded = {};
  
  if (bytes.length !== 9) {
    return decoded;
  }
  
  // Decode latitude (bytes 0-2)
  var latRaw = (bytes[0] << 16) | (bytes[1] << 8) | bytes[2];
  decoded.latitude = ((latRaw / 16777215.0) * 180.0) - 90.0;
  
  // Decode longitude (bytes 3-5)
  var lonRaw = (bytes[3] << 16) | (bytes[4] << 8) | bytes[5];
  decoded.longitude = ((lonRaw / 16777215.0) * 360.0) - 180.0;
  
  // Decode altitude (bytes 6-7) - signed int16
  var altRaw = (bytes[6] << 8) | bytes[7];
  if (altRaw & 0x8000) {
    decoded.altitude = altRaw - 0x10000;
  } else {
    decoded.altitude = altRaw;
  }
  
  // Decode HDOP (byte 8)
  decoded.hdop = bytes[8] / 10.0;
  
  return decoded;
}

// Test function (not used by TTN)
function testDecoder() {
  // Test payload: Lat: 52.520008, Lon: 13.404954, Alt: 50m, HDOP: 1.2
  var testBytes = [0x75, 0x8E, 0xBB, 0x83, 0xD3, 0xD0, 0x00, 0x32, 0x0C];
  
  console.log("Testing decoder with sample payload:");
  console.log("Input bytes:", testBytes);
  
  var result = decodeUplink({ bytes: testBytes });
  console.log("Decoded data:", result.data);
  
  // Expected output:
  // latitude: ~52.520008
  // longitude: ~13.404954
  // altitude: 50
  // hdop: 1.2
}
