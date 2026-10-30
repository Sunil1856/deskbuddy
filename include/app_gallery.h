#pragma once
// ============================================================================
// DeskBuddy — Photo & Video Media Gallery App
// ============================================================================
#include <Arduino.h>
#include "display_manager.h"
#include "storage.h"
#include "face_renderer.h"
#include "touch_input.h"

class AppGallery {
public:
    AppGallery();
    ~AppGallery();
    void begin(DisplayManager* dm, Storage* storage, FaceRenderer* face);
    
    void onEnter();
    void onExit();
    void update();
    void render();
    void handleTouch(TouchEvent event);
    
    // Photo management
    void setPhoto(const uint8_t* bitmap1024, bool saveToStorage = true);
    void clearPhoto();
    bool hasPhoto() const { return _hasPhoto; }

    // Video & Animation management
    bool setVideoStart(uint8_t totalFrames, uint8_t fps);
    bool setVideoFrame(uint8_t index, const uint8_t* bitmap1024);
    bool setVideoEnd(bool saveToStorage = true);
    void clearVideo();
    bool hasVideo() const { return _hasVideo; }
    bool isPlayingVideo() const { return _isPlayingVideo; }
    void toggleVideoPlay();
    uint8_t getVideoFramesCount() const { return _videoTotalFrames; }
    uint8_t getVideoFps() const { return _videoFps; }
    void setAutoCycle(bool isAutoCycle) { _isAutoCycle = isAutoCycle; }
    
    void setIP(const String& ip) { _localIP = ip; }

private:
    DisplayManager* _dm = nullptr;
    Storage* _storage = nullptr;
    FaceRenderer* _face = nullptr;
    
    uint8_t _photo[1024]; // 128x64 1-bit static photo
    bool _hasPhoto = false;
    String _localIP = "";

    // Video playback engine
    uint8_t* _videoFrames = nullptr; // Dynamically allocated in RAM
    uint8_t _videoTotalFrames = 0;
    uint8_t _videoFps = 10;
    uint8_t _videoCurrentFrame = 0;
    unsigned long _videoLastFrameTime = 0;
    bool _hasVideo = false;
    bool _isPlayingVideo = false;
    bool _isAutoCycle = false;
    
    void _renderEmpty();
    void _renderPhoto();
    void _renderVideo();
    void _saveVideoToSpiffs();
    void _loadVideoFromSpiffs();
};
