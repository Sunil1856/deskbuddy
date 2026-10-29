#pragma once
// ============================================================================
// DeskBuddy — Pi Server Monitor App (Pironman 5 Stats)
// ============================================================================
#include <Arduino.h>
#include "display_manager.h"
#include "storage.h"
#include "touch_input.h"
#include "config.h"

class AppServer {
public:
    void begin(DisplayManager* dm, Storage* storage);
    
    void onEnter();
    void onExit();
    void update();
    void render();
    void handleTouch(TouchEvent event);
    
    bool isConfigured() const {
        String url = SERVER_MONITOR_URL;
        if (_storage) {
            String saved = _storage->loadString("server_url", "");
            if (saved.length() > 0) url = saved;
        }
        return (url.length() > 0 && url.indexOf("192.168.x.x") < 0 && url.startsWith("http"));
    }
    
    bool isOnline() const { return _hasData; }

private:
    DisplayManager* _dm = nullptr;
    Storage* _storage = nullptr;
    
    bool _isFetching = false;
    unsigned long _lastFetchTime = 0;
    unsigned long _fetchIntervalMs = 5000;
    const unsigned long REFRESH_INTERVAL_MS = 5000;
    const unsigned long ERROR_BACKOFF_MS = 45000;
    
    // Server metrics
    float _cpuTemp = 0.0f;
    float _gpuTemp = 0.0f;
    float _cpuUsage = 0.0f;
    float _ramUsage = 0.0f;
    int _fanSpeed = 0;
    long _downloadBps = 0;
    long _uploadBps = 0;
    bool _hasData = false;
    
    void _fetchServerStats();
};
