/**
 * dedup.h
 * Time-window deduplicator.
 *
 * Prevents the same device (identified by its MAC address or beacon ID) from
 * being processed more than once within a configurable sliding time window.
 * The map is bounded to avoid unbounded RAM growth.
 */

#pragma once

#include <Arduino.h>
#include <map>

namespace Dedup {

/** Maximum number of entries held simultaneously. */
static constexpr int MAX_ENTRIES = 512;

class Deduplicator {
public:
    explicit Deduplicator(uint32_t windowSeconds)
        : _windowMs(windowSeconds * 1000UL) {}

    /**
     * Return true (and record the timestamp) if the key should be processed,
     * i.e. it has not been seen within the last _windowMs milliseconds.
     * Return false if it was seen recently and should be ignored.
     */
    bool shouldProcess(const String& key) {
        uint32_t now = millis();
        auto it = _seen.find(key);
        if (it != _seen.end()) {
            uint32_t elapsed = now - it->second;
            if (elapsed < _windowMs) return false; // within window → skip
            it->second = now;
            return true;
        }
        // New key
        if ((int)_seen.size() >= MAX_ENTRIES) evictOldest(now);
        _seen[key] = now;
        return true;
    }

    /** Remove all entries. */
    void clear() { _seen.clear(); }

    /** Change the deduplication window at runtime. */
    void setWindow(uint32_t seconds) { _windowMs = seconds * 1000UL; }

private:
    uint32_t _windowMs;
    std::map<String, uint32_t> _seen;

    /** Evict the single oldest entry to keep the map bounded. */
    void evictOldest(uint32_t now) {
        (void)now;
        if (_seen.empty()) return;
        auto oldest = _seen.begin();
        for (auto it = _seen.begin(); it != _seen.end(); ++it) {
            if (it->second < oldest->second) oldest = it;
        }
        _seen.erase(oldest);
    }
};

} // namespace Dedup
