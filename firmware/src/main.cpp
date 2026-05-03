/**
 * main.cpp
 * ESP32 BLE Attendance System – entry point.
 *
 * Hardware:  Any ESP32 dev board (ESP32-WROOM-32, ESP32-S3, etc.)
 *
 * What this file does:
 *  1. Mount LittleFS filesystem
 *  2. Load persisted student / attendance data
 *  3. Connect WiFi (STA mode → AP fallback)
 *  4. Sync time via NTP
 *  5. Initialise BLE scanner (NimBLE)
 *  6. Register WebSocket handler
 *  7. Register REST API routes
 *  8. Start HTTP server
 *  9. Loop: keep BLE scan cycling, check session timeout, clean WS clients
 *
 * Libraries (see platformio.ini):
 *   h2zero/NimBLE-Arduino    – lightweight BLE stack
 *   me-no-dev/ESP Async WebServer – async HTTP + WebSocket
 *   bblanchon/ArduinoJson    – JSON serialisation
 *   LittleFS                 – built-in with ESP32 Arduino core
 */

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <time.h>

#include "config.h"
#include "storage.h"
#include "attendance.h"
#include "ble_scanner.h"
#include "ws_manager.h"
#include "api_handlers.h"

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------

static AsyncWebServer server(HTTP_PORT);
static bool           ntpSynced = false;

// ---------------------------------------------------------------------------
// WiFi helpers
// ---------------------------------------------------------------------------

static bool connectSTA() {
    Serial.printf("[WiFi] Connecting to: %s\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    for (int i = 0; i < 20; i++) {
        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("[WiFi] Connected  IP: %s\n",
                          WiFi.localIP().toString().c_str());
            return true;
        }
        delay(500);
        Serial.print(".");
    }
    Serial.println("\n[WiFi] STA failed");
    return false;
}

static void startAP() {
    WiFi.mode(WIFI_AP);
    bool ok = (strlen(AP_PASSWORD) >= 8)
              ? WiFi.softAP(AP_SSID, AP_PASSWORD)
              : WiFi.softAP(AP_SSID);
    if (ok) {
        Serial.printf("[WiFi] AP started  SSID=%s  IP=%s\n",
                      AP_SSID, WiFi.softAPIP().toString().c_str());
    } else {
        Serial.println("[WiFi] AP start failed");
    }
}

// ---------------------------------------------------------------------------
// NTP sync
// ---------------------------------------------------------------------------

static void syncNTP() {
    Serial.printf("[NTP] Syncing from %s ...\n", NTP_SERVER);
    configTime(NTP_TIMEZONE_OFFSET_S, NTP_DST_OFFSET_S, NTP_SERVER);

    // Wait up to 10 s for NTP response
    struct tm timeinfo;
    for (int i = 0; i < 20; i++) {
        if (getLocalTime(&timeinfo)) {
            ntpSynced = true;
            char buf[64];
            strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
            Serial.printf("[NTP] Time synced: %s\n", buf);
            return;
        }
        delay(500);
    }
    Serial.println("[NTP] Sync failed – using uptime as timestamp");
}

// ---------------------------------------------------------------------------
// Arduino lifecycle
// ---------------------------------------------------------------------------

void setup() {
    Serial.begin(115200);
    Serial.println("\n\n=== ESP32 BLE Attendance System ===");

    // 1. Mount filesystem
    if (!Storage::begin()) {
        Serial.println("[Boot] WARNING: filesystem unavailable – data will not persist");
    }

    // 2. Load persisted data
    Attendance::init();

    // 3. WiFi
    if (!connectSTA()) {
        startAP();
    }

    // 4. NTP (only useful when STA is connected)
    if (WiFi.status() == WL_CONNECTED) {
        syncNTP();
    }

    // 5. BLE scanner
    BLEScanner::begin([](const BLEScanner::DeviceEvent& ev) {
        Attendance::processDevice(ev.mac, ev.rssi,
                                  ev.beaconId, ev.name,
                                  ev.timestamp);
    });
    BLEScanner::startScan();

    // 6. WebSocket
    WSManager::begin(server);

    // 7. REST API
    ApiHandlers::registerRoutes(server);

    // 8. Start server
    server.begin();
    Serial.printf("[Boot] HTTP server on port %d\n", HTTP_PORT);
    Serial.println("[Boot] Open http://" +
                   (WiFi.getMode() == WIFI_AP
                        ? WiFi.softAPIP().toString()
                        : WiFi.localIP().toString()) +
                   "/ in your browser");
    Serial.println("[Boot] Setup complete\n");
}

void loop() {
    // BLE scan cycles automatically with the restart=true flag in startScan().
    // Nothing extra required here for scanning.

    static unsigned long lastSecond    = 0;
    static unsigned long lastWsClean   = 0;

    unsigned long now = millis();

    // Session timeout check – once per second
    if (now - lastSecond >= 1000UL) {
        lastSecond = now;
        Attendance::checkTimeout(SESSION_TIMEOUT_S);
    }

    // WebSocket client cleanup – every 30 s
    if (now - lastWsClean >= 30000UL) {
        lastWsClean = now;
        WSManager::cleanup();
    }

    // Re-attempt NTP sync once after boot if STA connected late
    if (!ntpSynced && WiFi.status() == WL_CONNECTED) {
        syncNTP();
    }

    delay(10); // yield to FreeRTOS tasks
}
