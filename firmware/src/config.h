/**
 * config.h
 * ESP32 BLE Attendance System – compile-time configuration.
 *
 * Edit this file before flashing. All WiFi credentials, thresholds, and
 * behavioural tuning knobs live here so that no other source file needs
 * to be changed for a deployment-specific adjustment.
 *
 * SECURITY: Do NOT commit real credentials to version control. Consider
 * using a captive-portal provisioning flow for production deployments.
 */

#pragma once

// ---------------------------------------------------------------------------
// WiFi – Station mode (connect to existing router)
// ---------------------------------------------------------------------------
#define WIFI_SSID       "madhav@vianet"
#define WIFI_PASSWORD   "success$111$"

// --------------------------------------------------------------------------
// WiFi – Access Point fallback (created when STA fails)
// ---------------------------------------------------------------------------
#define AP_SSID         "BLE_ATTENDANCE"
#define AP_PASSWORD     "12345678"          // min 8 chars; "" = open network

// ---------------------------------------------------------------------------
// Web server
// ---------------------------------------------------------------------------
#define HTTP_PORT       80
#define WS_PATH         "/ws"

// ---------------------------------------------------------------------------
// Authentication – Bearer token for mutation endpoints.
// Set "" to disable authentication entirely.
// Must match AUTH_TOKEN in the dashboard (index.html).
// ---------------------------------------------------------------------------
#define AUTH_TOKEN      "esp32-attend-secret"

// ---------------------------------------------------------------------------
// BLE scanning parameters
// ---------------------------------------------------------------------------
// Duration of each active scan cycle (seconds). After this the scan restarts.
#define BLE_SCAN_DURATION_S   5

// NimBLE units: 1 unit = 0.625 ms
#define BLE_SCAN_INTERVAL     160    // 100 ms
#define BLE_SCAN_WINDOW       80     // 50 ms  (duty cycle 50 %)

// RSSI threshold (dBm). Devices weaker than this are ignored.
#define RSSI_THRESHOLD        -80

// ---------------------------------------------------------------------------
// Deduplication
// A device MAC (or beacon ID) is processed at most once per DEDUP_WINDOW_S
// seconds, preventing duplicate attendance marks from rapid re-advertisements.
// ---------------------------------------------------------------------------
#define DEDUP_WINDOW_S        30     // seconds

// ---------------------------------------------------------------------------
// iBeacon UUID filter
// If non-empty, only beacons whose UUID matches this string are accepted.
// Format: "XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX" (upper-case).
// Leave "" to accept all iBeacon advertisements.
// ---------------------------------------------------------------------------
#define IBEACON_UUID_FILTER   ""

// ---------------------------------------------------------------------------
// Data limits (trade off against available heap ~300 KB on ESP32)
// ---------------------------------------------------------------------------
#define MAX_STUDENTS          150
#define MAX_ATTENDANCE        1000
#define MAX_SCAN_LOGS         300

// ---------------------------------------------------------------------------
// Session
// ---------------------------------------------------------------------------
// Auto-stop after this many seconds (0 = never time out).
#define SESSION_TIMEOUT_S     3600UL   // 1 hour

// ---------------------------------------------------------------------------
// NTP time synchronisation
// ---------------------------------------------------------------------------
#define NTP_SERVER            "pool.ntp.org"
#define NTP_TIMEZONE_OFFSET_S 0        // UTC offset in seconds (e.g. 19800 = IST)
#define NTP_DST_OFFSET_S      0        // DST offset in seconds
