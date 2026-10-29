#pragma once
// ============================================================================
// DeskBuddy — Photo Gallery / Live Photo Frame App
// ============================================================================
#include <Arduino.h>
#include "display_manager.h"
#include "storage.h"
#include "face_renderer.h"
#include "touch_input.h"

class AppGallery {
public:
    AppGallery();
    void begin(DisplayManager* dm, Storage* storage, FaceRenderer* face);
    
    void onEnter();
    void onExit();
    void update();
    void render();
    void handleTouch(TouchEvent event);
    
    // Updates photo in RAM and storage
    void setPhoto(const uint8_t* bitmap1024, bool saveToStorage = true);
    bool hasPhoto() const { return _hasPhoto; }
    
    void setIP(const String& ip) { _localIP = ip; }

private:
    DisplayManager* _dm = nullptr;
    Storage* _storage = nullptr;
    FaceRenderer* _face = nullptr;
    
    uint8_t _photo[1024]; // 128x64 1-bit bitmap
    bool _hasPhoto = false;
    String _localIP = "";
    
    void _renderEmpty();
    void _renderPhoto();
};
