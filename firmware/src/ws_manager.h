/**
 * ws_manager.h
 * WebSocket broadcast manager.
 *
 * Wraps ESPAsyncWebServer's AsyncWebSocket and provides a simple typed
 * broadcast API so other modules do not need to know about WebSocket frames.
 *
 * Message format (JSON):
 *   { "event": "<type>", "data": <payload_object> }
 *
 * Event types:
 *   "scan"       – raw BLE device detected
 *   "attendance" – student marked present
 *   "session"    – session state changed
 *   "ping"       – keep-alive (sent by client; ignored here)
 */

#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

namespace WSManager {

/** Attach the WebSocket handler to the AsyncWebServer instance. */
void begin(AsyncWebServer& server);

/**
 * Broadcast an event to all connected WebSocket clients.
 *
 * @param event  One of "scan", "attendance", "session"
 * @param json   JSON string of the data payload (already serialised)
 */
void broadcastEvent(const String& event, const String& json);

/** Cleanup disconnected clients (call from loop() periodically). */
void cleanup();

} // namespace WSManager
