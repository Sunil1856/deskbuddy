#pragma once
// ============================================================================
// DeskBuddy — WiFi Manager & Captive Portal
// ============================================================================
#include <Arduino.h>
#include "display_manager.h"
#include "storage.h"

enum class WifiState {
    IDLE,
    CONNECTING,
    CONNECTED,
    FAILED,
    PORTAL_ACTIVE
};

class AppGallery;
typedef void (*PhotoUploadedCallback)();

class WifiManager {
public:
    void begin(DisplayManager* dm, Storage* storage);
    void update(); // call in main loop for DNS server & web requests
    
    // Connect to saved WiFi or start portal if none
    void autoConnect();
    
    // Force start the captive portal setup
    void startPortal();
    
    WifiState getState() const { return _state; }
    bool isConnected() const { return _state == WifiState::CONNECTED; }
    String getIP() const;
    
    // Manually disconnect
    void disconnect();
    
    // Photo Studio attachment
    void setGallery(AppGallery* gallery) { _gallery = gallery; }
    void setOnPhotoUploaded(PhotoUploadedCallback cb) { _onPhotoUploaded = cb; }
    
private:
    DisplayManager* _dm = nullptr;
    Storage* _storage = nullptr;
    AppGallery* _gallery = nullptr;
    PhotoUploadedCallback _onPhotoUploaded = nullptr;
    
    WifiState _state = WifiState::IDLE;
    unsigned long _connectStartTime = 0;
    bool _hasConnectedOnce = false;
    
    void _startAP();
    void _setupWebRoutes();
    void _setupStationWebRoutes();
    void _drawConnectingScreen();
    void _drawPortalScreen();
};
