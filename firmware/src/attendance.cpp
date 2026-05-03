/**
 * attendance.cpp
 * Attendance engine implementation.
 */

#include "attendance.h"
#include "ws_manager.h"
#include <ArduinoJson.h>
#include <time.h>

namespace Attendance {

// ---------------------------------------------------------------------------
// Public state
// ---------------------------------------------------------------------------

bool          sessionActive  = false;
String        sessionId      = "";
String        sessionClass   = "";
time_t        sessionStart   = 0;
time_t        sessionEnd     = 0;

Storage::Student          students[MAX_STUDENTS];
int                       studentCount = 0;
Storage::AttendanceRecord attendance[MAX_ATTENDANCE];
int                       attendanceCount = 0;

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

static String makeSessionId() {
    time_t now = time(nullptr);
    char buf[20];
    snprintf(buf, sizeof(buf), "sess_%ld", (long)now);
    return String(buf);
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void init() {
    studentCount    = Storage::loadStudents(students, MAX_STUDENTS);
    attendanceCount = Storage::loadAttendance(attendance, MAX_ATTENDANCE);
    Serial.printf("[Attendance] Loaded %d students, %d attendance records\n",
                  studentCount, attendanceCount);
}

// ---------------------------------------------------------------------------
// Session management
// ---------------------------------------------------------------------------

bool startSession(const String& className) {
    if (sessionActive) return false;

    time_t now    = time(nullptr);
    sessionStart  = (now > 1000000000L) ? now : (time_t)(millis() / 1000UL);
    sessionId     = makeSessionId();
    sessionClass  = className.length() ? className : "Attendance Session";
    sessionEnd    = 0;
    sessionActive = true;

    Serial.printf("[Attendance] Session started: %s (%s)\n",
                  sessionId.c_str(), sessionClass.c_str());

    String json;
    sessionToJson(json);
    WSManager::broadcastEvent("session", json);
    return true;
}

bool stopSession() {
    if (!sessionActive) return false;

    time_t now  = time(nullptr);
    sessionEnd  = (now > 1000000000L) ? now : (time_t)(millis() / 1000UL);
    sessionActive = false;

    Serial.printf("[Attendance] Session stopped: %s (duration %ld s)\n",
                  sessionId.c_str(), (long)(sessionEnd - sessionStart));

    String json;
    sessionToJson(json);
    WSManager::broadcastEvent("session", json);
    return true;
}

void checkTimeout(unsigned long timeoutSeconds) {
    if (!sessionActive || timeoutSeconds == 0) return;
    time_t now     = time(nullptr);
    time_t current = (now > 1000000000L) ? now : (time_t)(millis() / 1000UL);
    if ((unsigned long)(current - sessionStart) >= timeoutSeconds) {
        Serial.println("[Attendance] Session auto-timeout");
        stopSession();
    }
}

// ---------------------------------------------------------------------------
// Student management
// ---------------------------------------------------------------------------

int findStudentByDevice(const String& deviceId) {
    for (int i = 0; i < studentCount; i++) {
        if (students[i].deviceId.equalsIgnoreCase(deviceId)) return i;
    }
    return -1;
}

int findStudentByBeacon(const String& beaconId) {
    if (beaconId.length() == 0) return -1;
    for (int i = 0; i < studentCount; i++) {
        if (students[i].beaconId == beaconId) return i;
    }
    return -1;
}

bool registerStudent(const String& deviceId, const String& name,
                     const String& beaconId, const String& rollNumber) {
    // Duplicate check – same MAC
    if (findStudentByDevice(deviceId) >= 0) return false;
    // Duplicate check – same beacon ID
    if (beaconId.length() > 0 && findStudentByBeacon(beaconId) >= 0) return false;
    if (studentCount >= MAX_STUDENTS) return false;

    students[studentCount].deviceId   = deviceId;
    students[studentCount].name       = name;
    students[studentCount].beaconId   = beaconId;
    students[studentCount].rollNumber = rollNumber;
    studentCount++;

    Storage::saveStudents(students, studentCount);
    Serial.printf("[Attendance] Registered: %s  MAC=%s  Beacon=%s\n",
                  name.c_str(), deviceId.c_str(), beaconId.c_str());
    return true;
}

bool removeStudent(int index) {
    if (index < 0 || index >= studentCount) return false;
    for (int i = index; i < studentCount - 1; i++) {
        students[i] = students[i + 1];
    }
    studentCount--;
    Storage::saveStudents(students, studentCount);
    return true;
}

// ---------------------------------------------------------------------------
// Scan processing
// ---------------------------------------------------------------------------

bool processDevice(const String& mac, int rssi,
                   const String& beaconId, const String& name,
                   time_t timestamp) {
    // Broadcast raw scan event regardless of session / registration
    {
        DynamicJsonDocument doc(256);
        doc["mac"]       = mac;
        doc["rssi"]      = rssi;
        doc["name"]      = name;
        doc["beacon_id"] = beaconId;
        doc["timestamp"] = (long)timestamp;
        String scanJson;
        serializeJson(doc, scanJson);
        WSManager::broadcastEvent("scan", scanJson);
    }

    // Append to scan log (background, best-effort)
    {
        Storage::ScanLog log;
        log.mac       = mac;
        log.rssi      = rssi;
        log.timestamp = timestamp;
        log.beaconId  = beaconId;
        log.name      = name;
        Storage::appendScanLog(log);
    }

    if (!sessionActive) return false;

    // Resolve student
    int idx = -1;
    if (beaconId.length() > 0) idx = findStudentByBeacon(beaconId);
    if (idx < 0) idx = findStudentByDevice(mac);
    if (idx < 0) return false;  // unknown device

    const String& studentName = students[idx].name;

    // Deduplicate attendance within this session
    for (int i = 0; i < attendanceCount; i++) {
        if (attendance[i].sessionId == sessionId &&
            attendance[i].deviceId  == mac) {
            return false;
        }
    }

    if (attendanceCount >= MAX_ATTENDANCE) {
        Serial.println("[Attendance] Buffer full");
        return false;
    }

    attendance[attendanceCount].deviceId  = mac;
    attendance[attendanceCount].sessionId = sessionId;
    attendance[attendanceCount].timestamp = timestamp;
    attendance[attendanceCount].name      = studentName;
    attendanceCount++;

    Storage::saveAttendance(attendance, attendanceCount);
    Serial.printf("[Attendance] Marked: %s (%s)  RSSI=%d  session=%s\n",
                  studentName.c_str(), mac.c_str(), rssi, sessionId.c_str());

    // Broadcast attendance event
    {
        DynamicJsonDocument doc(256);
        doc["name"]       = studentName;
        doc["mac"]        = mac;
        doc["beacon_id"]  = beaconId;
        doc["session_id"] = sessionId;
        doc["rssi"]       = rssi;
        doc["timestamp"]  = (long)timestamp;
        String attJson;
        serializeJson(doc, attJson);
        WSManager::broadcastEvent("attendance", attJson);
    }

    return true;
}

// ---------------------------------------------------------------------------
// JSON serialisation
// ---------------------------------------------------------------------------

void studentsToJson(String& out) {
    DynamicJsonDocument doc(MAX_STUDENTS * 120 + 64);
    JsonArray arr = doc.to<JsonArray>();
    for (int i = 0; i < studentCount; i++) {
        JsonObject obj = arr.createNestedObject();
        obj["id"]          = i;
        obj["device_id"]   = students[i].deviceId;
        obj["name"]        = students[i].name;
        obj["beacon_id"]   = students[i].beaconId;
        obj["roll_number"] = students[i].rollNumber;
    }
    serializeJson(doc, out);
}

void attendanceToJson(String& out) {
    DynamicJsonDocument doc(attendanceCount * 120 + 64);
    JsonArray arr = doc.to<JsonArray>();
    for (int i = 0; i < attendanceCount; i++) {
        JsonObject obj = arr.createNestedObject();
        obj["device_id"]  = attendance[i].deviceId;
        obj["session_id"] = attendance[i].sessionId;
        obj["timestamp"]  = (long)attendance[i].timestamp;
        obj["name"]       = attendance[i].name;
    }
    serializeJson(doc, out);
}

void sessionToJson(String& out) {
    DynamicJsonDocument doc(256);
    doc["active"]      = sessionActive;
    doc["session_id"]  = sessionId;
    doc["class_name"]  = sessionClass;
    doc["start"]       = (long)sessionStart;
    doc["end"]         = (long)sessionEnd;
    doc["count"]       = attendanceCount;
    serializeJson(doc, out);
}

void statsToJson(String& out) {
    DynamicJsonDocument doc(256);
    doc["total_students"]   = studentCount;
    doc["total_attendance"] = attendanceCount;
    doc["session_active"]   = sessionActive;
    doc["session_id"]       = sessionId;
    doc["class_name"]       = sessionClass;
    // Count current session attendance
    int sessionCount = 0;
    for (int i = 0; i < attendanceCount; i++) {
        if (attendance[i].sessionId == sessionId) sessionCount++;
    }
    doc["session_count"] = sessionCount;
    serializeJson(doc, out);
}

} // namespace Attendance
