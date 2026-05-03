/**
 * storage.h
 * LittleFS JSON persistence layer.
 *
 * Data model
 * ----------
 * /students.json  – JSON array of student objects
 *   { "d": "<mac>", "n": "<name>", "b": "<beacon_id>", "r": "<roll>" }
 *
 * /attendance.json – JSON array of attendance records
 *   { "d": "<mac_or_beacon>", "s": "<session_id>", "t": <unix_ts> }
 *
 * /scanlogs.json – JSON array of raw scan events (ring buffer, MAX_SCAN_LOGS)
 *   { "d": "<mac>", "r": <rssi>, "t": <unix_ts>, "b": "<beacon_id>" }
 *
 * Short keys are used to save flash space and heap allocation during parse.
 */

#pragma once

#include <Arduino.h>
#include "config.h"

namespace Storage {

// ---------------------------------------------------------------------------
// Data structures
// ---------------------------------------------------------------------------

struct Student {
    String deviceId;    // BLE MAC address "AA:BB:CC:DD:EE:FF"
    String name;
    String beaconId;    // minor value string, e.g. "0042" (or "" if MAC-only)
    String rollNumber;  // optional roll / student ID
};

struct AttendanceRecord {
    String deviceId;    // MAC or beaconId used as key
    String sessionId;
    time_t timestamp;
    String name;        // denormalised for fast JSON output
};

struct ScanLog {
    String mac;
    int    rssi;
    time_t timestamp;
    String beaconId;
    String name;
};

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

/** Mount LittleFS. Returns false if unavailable (even after format attempt). */
bool begin();

/** Wipe all stored data (format LittleFS). */
void format();

// ---------------------------------------------------------------------------
// Students
// ---------------------------------------------------------------------------

int  loadStudents(Student dest[], int maxCount);
bool saveStudents(const Student src[], int count);

// ---------------------------------------------------------------------------
// Attendance
// ---------------------------------------------------------------------------

int  loadAttendance(AttendanceRecord dest[], int maxCount);
bool saveAttendance(const AttendanceRecord src[], int count);

// ---------------------------------------------------------------------------
// Scan logs (append-only ring, trimmed to MAX_SCAN_LOGS)
// ---------------------------------------------------------------------------

bool appendScanLog(const ScanLog& log);

} // namespace Storage
