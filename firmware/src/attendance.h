/**
 * attendance.h
 * Attendance session and business logic.
 *
 * This module owns the in-memory student list, active session state, and
 * attendance records. It receives BLE device events from ble_scanner and
 * marks students as present.
 */

#pragma once

#include <Arduino.h>
#include "storage.h"
#include "config.h"
#include "ws_manager.h"

namespace Attendance {

// ---------------------------------------------------------------------------
// Session state (readable by API handlers)
// ---------------------------------------------------------------------------

extern bool          sessionActive;
extern String        sessionId;
extern String        sessionClass;
extern time_t        sessionStart;
extern time_t        sessionEnd;

extern Storage::Student          students[MAX_STUDENTS];
extern int                       studentCount;
extern Storage::AttendanceRecord attendance[MAX_ATTENDANCE];
extern int                       attendanceCount;

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

/** Load persisted data from LittleFS. Call once after Storage::begin(). */
void init();

// ---------------------------------------------------------------------------
// Session management
// ---------------------------------------------------------------------------

/**
 * Start a new session.
 * @param className  Optional descriptive name (e.g. "CS101").
 * @return false if a session is already active.
 */
bool startSession(const String& className = "");

/**
 * Stop the current session.
 * @return false if no session was active.
 */
bool stopSession();

/**
 * Check and apply the automatic session timeout. Call from loop() every second.
 */
void checkTimeout(unsigned long timeoutSeconds);

// ---------------------------------------------------------------------------
// Student management
// ---------------------------------------------------------------------------

/**
 * Register a new student.
 * Returns false if duplicate or limit reached.
 */
bool registerStudent(const String& deviceId, const String& name,
                     const String& beaconId   = "",
                     const String& rollNumber = "");

/**
 * Remove a student by index. Returns false if index out of range.
 */
bool removeStudent(int index);

/**
 * Find a student index by MAC address or beacon ID.
 * Returns -1 if not found.
 */
int findStudentByDevice(const String& deviceId);
int findStudentByBeacon(const String& beaconId);

// ---------------------------------------------------------------------------
// Scan processing (called by BLE scanner callback)
// ---------------------------------------------------------------------------

/**
 * Process a detected device. Marks the matched student as present if:
 *   - a session is active
 *   - RSSI is above RSSI_THRESHOLD (already filtered by scanner)
 *   - the device maps to a registered student
 *   - the student is not already marked for this session
 *
 * @param mac       BLE MAC address string
 * @param rssi      Signal strength (dBm)
 * @param beaconId  Beacon minor ID string ("0042") or "" if MAC-only
 * @param name      Advertised name or ""
 * @return true if attendance was newly marked
 */
bool processDevice(const String& mac, int rssi,
                   const String& beaconId, const String& name,
                   time_t timestamp);

// ---------------------------------------------------------------------------
// JSON serialisation helpers
// ---------------------------------------------------------------------------

void studentsToJson(String& out);
void attendanceToJson(String& out);
void sessionToJson(String& out);
void statsToJson(String& out);

} // namespace Attendance
