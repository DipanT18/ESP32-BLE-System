/**
 * storage.cpp
 * LittleFS JSON persistence implementation.
 */

#include "storage.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

namespace Storage {

static const char* STUDENTS_FILE   = "/students.json";
static const char* ATTENDANCE_FILE = "/attendance.json";
static const char* SCANLOGS_FILE   = "/scanlogs.json";

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

bool begin() {
    if (!LittleFS.begin(true)) {          // true = format on fail
        Serial.println("[Storage] LittleFS mount failed – tried format");
        if (!LittleFS.begin(false)) {
            Serial.println("[Storage] LittleFS unavailable");
            return false;
        }
    }
    Serial.println("[Storage] LittleFS mounted OK");
    return true;
}

void format() {
    LittleFS.format();
}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

static String readFile(const char* path) {
    File f = LittleFS.open(path, "r");
    if (!f || f.isDirectory()) return "[]";
    String s = f.readString();
    f.close();
    return s;
}

static bool writeFile(const char* path, const String& data) {
    File f = LittleFS.open(path, "w");
    if (!f) {
        Serial.printf("[Storage] Cannot write %s\n", path);
        return false;
    }
    f.print(data);
    f.close();
    return true;
}

// ---------------------------------------------------------------------------
// Students
// ---------------------------------------------------------------------------

int loadStudents(Student dest[], int maxCount) {
    // 150 students * ~80 bytes + overhead ≈ 14 KB
    DynamicJsonDocument doc(16384);
    DeserializationError err = deserializeJson(doc, readFile(STUDENTS_FILE));
    if (err) {
        Serial.printf("[Storage] loadStudents parse error: %s\n", err.c_str());
        return 0;
    }
    JsonArray arr = doc.as<JsonArray>();
    int count = 0;
    for (JsonObject obj : arr) {
        if (count >= maxCount) break;
        dest[count].deviceId   = obj["d"].as<String>();
        dest[count].name       = obj["n"].as<String>();
        dest[count].beaconId   = obj["b"].as<String>();
        dest[count].rollNumber = obj["r"].as<String>();
        count++;
    }
    return count;
}

bool saveStudents(const Student src[], int count) {
    DynamicJsonDocument doc(16384);
    JsonArray arr = doc.to<JsonArray>();
    for (int i = 0; i < count; i++) {
        JsonObject obj = arr.createNestedObject();
        obj["d"] = src[i].deviceId;
        obj["n"] = src[i].name;
        obj["b"] = src[i].beaconId;
        obj["r"] = src[i].rollNumber;
    }
    String output;
    serializeJson(doc, output);
    return writeFile(STUDENTS_FILE, output);
}

// ---------------------------------------------------------------------------
// Attendance
// ---------------------------------------------------------------------------

int loadAttendance(AttendanceRecord dest[], int maxCount) {
    // 1000 records * ~80 bytes + overhead ≈ 90 KB
    DynamicJsonDocument doc(98304);
    DeserializationError err = deserializeJson(doc, readFile(ATTENDANCE_FILE));
    if (err) {
        Serial.printf("[Storage] loadAttendance parse error: %s\n", err.c_str());
        return 0;
    }
    JsonArray arr = doc.as<JsonArray>();
    int count = 0;
    for (JsonObject obj : arr) {
        if (count >= maxCount) break;
        dest[count].deviceId  = obj["d"].as<String>();
        dest[count].sessionId = obj["s"].as<String>();
        dest[count].timestamp = (time_t)obj["t"].as<long>();
        dest[count].name      = obj["n"].as<String>();
        count++;
    }
    return count;
}

bool saveAttendance(const AttendanceRecord src[], int count) {
    DynamicJsonDocument doc(98304);
    JsonArray arr = doc.to<JsonArray>();
    for (int i = 0; i < count; i++) {
        JsonObject obj = arr.createNestedObject();
        obj["d"] = src[i].deviceId;
        obj["s"] = src[i].sessionId;
        obj["t"] = (long)src[i].timestamp;
        obj["n"] = src[i].name;
    }
    String output;
    serializeJson(doc, output);
    return writeFile(ATTENDANCE_FILE, output);
}

// ---------------------------------------------------------------------------
// Scan logs – append with ring-buffer trim
// ---------------------------------------------------------------------------

bool appendScanLog(const ScanLog& log) {
    // Load existing logs; reinitialise with empty array on parse error
    DynamicJsonDocument doc(32768);
    String existing = readFile(SCANLOGS_FILE);
    DeserializationError err = deserializeJson(doc, existing);
    if (err) {
        Serial.printf("[Storage] appendScanLog parse error: %s – reinitialising\n",
                      err.c_str());
        doc.to<JsonArray>(); // reset to empty array
    }
    JsonArray arr = doc.as<JsonArray>();

    // Trim to leave room for 1 more
    while ((int)arr.size() >= MAX_SCAN_LOGS) {
        arr.remove(0);
    }

    JsonObject obj = arr.createNestedObject();
    obj["d"] = log.mac;
    obj["r"] = log.rssi;
    obj["t"] = (long)log.timestamp;
    obj["b"] = log.beaconId;
    obj["n"] = log.name;

    String output;
    serializeJson(doc, output);
    return writeFile(SCANLOGS_FILE, output);
}

} // namespace Storage
