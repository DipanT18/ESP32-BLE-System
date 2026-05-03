/**
 * ibeacon.h
 * Apple iBeacon advertisement parser.
 *
 * An iBeacon is identified by Apple's company ID (0x004C) in the BLE
 * manufacturer-specific data, followed by type 0x02 and length 0x15,
 * then 16-byte UUID, 2-byte major, 2-byte minor, and 1-byte TX power.
 */

#pragma once

#include <Arduino.h>

namespace IBeacon {

/**
 * Parsed iBeacon payload.
 * beacon_id is set to "<UUID>/<major>/<minor>" when valid,
 * or the minor value as a zero-padded 4-digit string (student identifier).
 */
struct Payload {
    bool    valid   = false;
    String  uuid;           // "XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX"
    uint16_t major  = 0;
    uint16_t minor  = 0;
    int8_t  txPower = 0;    // dBm at 1 m (signed)

    /** Returns the minor value as a zero-padded 4-digit string, e.g. "0042". */
    String beaconId() const {
        char buf[5];
        snprintf(buf, sizeof(buf), "%04u", (unsigned)minor);
        return String(buf);
    }

    /** Returns full composite ID: "<UUID>/<major>/<minor>". */
    String fullId() const {
        return uuid + "/" + String(major) + "/" + String(minor);
    }
};

/**
 * Parse raw manufacturer-specific data bytes into an iBeacon payload.
 *
 * @param data   Pointer to the raw bytes (starting at the 2-byte company ID).
 * @param length Number of bytes available.
 * @return Filled Payload; valid == false if data does not match iBeacon format.
 */
Payload parse(const uint8_t* data, size_t length);

/**
 * Return true when the UUID in @p payload matches @p filter.
 * Comparison is case-insensitive. If filter is empty, always returns true.
 */
bool uuidMatches(const Payload& payload, const String& filter);

} // namespace IBeacon
