/**
 * ble_scanner.h
 * ESP32 BLE GAP scanner – continuously scans for BLE advertising packets,
 * applies RSSI filtering, parses iBeacon payloads, deduplicates events, and
 * fires a callback for each accepted device.
 *
 * Uses NimBLE-Arduino (lighter than the default ESP32 BLE stack).
 * Library: h2zero/NimBLE-Arduino (included via platformio.ini).
 */

#pragma once

#include <Arduino.h>
#include <functional>
#include "ibeacon.h"
#include "dedup.h"
#include "config.h"

namespace BLEScanner {

/**
 * A normalised device event emitted for each accepted BLE advertisement.
 */
struct DeviceEvent {
    String  mac;         // "AA:BB:CC:DD:EE:FF"
    int     rssi;        // dBm (negative)
    String  name;        // advertised local name or ""
    bool    hasBeacon;   // true if iBeacon payload was decoded
    String  beaconId;    // minor value as 4-digit string (or "" if none)
    String  beaconUuid;  // UUID string (or "" if none)
    uint16_t major;
    uint16_t minor;
    time_t  timestamp;   // Unix epoch (from NTP, or millis()/1000 if no NTP)
};

using EventCallback = std::function<void(const DeviceEvent&)>;

/**
 * Initialise the BLE stack and deduplicator. Call once from setup().
 * @param callback  Called for every accepted (deduplicated) BLE advertisement.
 */
void begin(EventCallback callback);

/**
 * Start a scan cycle. Call once after begin().
 * The scanner will automatically restart after each BLE_SCAN_DURATION_S cycle.
 */
void startScan();

/**
 * Stop scanning. Use before BLE advertising (not used in this system but
 * provided for completeness).
 */
void stopScan();

/** True when a scan cycle is currently active. */
bool isScanning();

} // namespace BLEScanner
