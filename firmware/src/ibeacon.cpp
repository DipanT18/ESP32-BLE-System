/**
 * ibeacon.cpp
 * Apple iBeacon advertisement parser implementation.
 */

#include "ibeacon.h"

namespace IBeacon {

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

static constexpr uint8_t APPLE_COMPANY_ID_LO = 0x4C; // little-endian LSB
static constexpr uint8_t APPLE_COMPANY_ID_HI = 0x00; // little-endian MSB
static constexpr uint8_t IBEACON_TYPE         = 0x02;
static constexpr uint8_t IBEACON_LENGTH       = 0x15; // 21 bytes of payload

// ---------------------------------------------------------------------------
// Helper: format 16 raw bytes as "XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX"
// ---------------------------------------------------------------------------
static String formatUuid(const uint8_t* b) {
    char buf[37];
    snprintf(buf, sizeof(buf),
             "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
             b[0],  b[1],  b[2],  b[3],
             b[4],  b[5],
             b[6],  b[7],
             b[8],  b[9],
             b[10], b[11], b[12], b[13], b[14], b[15]);
    return String(buf);
}

// ---------------------------------------------------------------------------
// parse()
// ---------------------------------------------------------------------------

Payload parse(const uint8_t* data, size_t length) {
    Payload p;

    // Minimum: 2 (company ID) + 1 (type) + 1 (length) + 21 (payload) = 25
    if (length < 25) return p;

    // Check Apple company ID (0x004C, little-endian → lo=0x4C, hi=0x00)
    if (data[0] != APPLE_COMPANY_ID_LO || data[1] != APPLE_COMPANY_ID_HI) return p;

    // Check iBeacon type and payload length
    if (data[2] != IBEACON_TYPE || data[3] != IBEACON_LENGTH) return p;

    // --- parse UUID (bytes 4..19) ---
    p.uuid = formatUuid(data + 4);

    // --- major (bytes 20..21, big-endian) ---
    p.major = ((uint16_t)data[20] << 8) | data[21];

    // --- minor (bytes 22..23, big-endian) ---
    p.minor = ((uint16_t)data[22] << 8) | data[23];

    // --- TX power (byte 24, signed) ---
    p.txPower = (int8_t)data[24];

    p.valid = true;
    return p;
}

// ---------------------------------------------------------------------------
// uuidMatches()
// ---------------------------------------------------------------------------

bool uuidMatches(const Payload& payload, const String& filter) {
    if (filter.length() == 0) return true;
    String a = payload.uuid;
    String b = filter;
    a.toUpperCase();
    b.toUpperCase();
    return a == b;
}

} // namespace IBeacon
