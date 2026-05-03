/**
 * ws_manager.cpp
 * WebSocket broadcast manager implementation.
 */

#include "ws_manager.h"
#include "config.h"

namespace WSManager {

// Static allocation – avoids heap fragmentation and a destructor mismatch
static AsyncWebSocket s_wsInstance(WS_PATH);
static AsyncWebSocket* s_ws = &s_wsInstance;

void begin(AsyncWebServer& server) {
    s_ws->onEvent([](AsyncWebSocket* /*server*/,
                     AsyncWebSocketClient* client,
                     AwsEventType type,
                     void* /*arg*/,
                     uint8_t* data,
                     size_t len) {
        if (type == WS_EVT_CONNECT) {
            Serial.printf("[WS] Client #%u connected from %s\n",
                          client->id(),
                          client->remoteIP().toString().c_str());
        } else if (type == WS_EVT_DISCONNECT) {
            Serial.printf("[WS] Client #%u disconnected\n", client->id());
        } else if (type == WS_EVT_DATA) {
            // Accept "ping" messages from clients; ignore everything else
            (void)data; (void)len;
        } else if (type == WS_EVT_ERROR) {
            Serial.printf("[WS] Client #%u error\n", client->id());
        }
    });

    server.addHandler(s_ws);
    Serial.printf("[WS] WebSocket registered at %s\n", WS_PATH);
}

void broadcastEvent(const String& event, const String& json) {
    if (!s_ws || s_ws->count() == 0) return;

    // Build envelope: {"event":"<type>","data":<payload>}
    String msg;
    msg.reserve(event.length() + json.length() + 20);
    msg  = "{\"event\":\"";
    msg += event;
    msg += "\",\"data\":";
    msg += json;
    msg += "}";

    s_ws->textAll(msg);
}

void cleanup() {
    if (s_ws) s_ws->cleanupClients();
}

} // namespace WSManager
