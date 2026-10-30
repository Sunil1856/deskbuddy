// ============================================================================
// DeskBuddy — Photo & Video Media Gallery Implementation
// ============================================================================
#include "app_gallery.h"
#include "icons.h"
#include <SPIFFS.h>

AppGallery::AppGallery() {
    memset(_photo, 0, sizeof(_photo));
    _videoFrames = nullptr;
    _videoTotalFrames = 0;
    _videoFps = 10;
    _videoCurrentFrame = 0;
    _videoLastFrameTime = 0;
    _hasVideo = false;
    _isPlayingVideo = false;
    _isAutoCycle = false;
}

AppGallery::~AppGallery() {
    if (_videoFrames) {
        free(_videoFrames);
        _videoFrames = nullptr;
    }
}

void AppGallery::begin(DisplayManager* dm, Storage* storage, FaceRenderer* face) {
    _dm = dm;
    _storage = storage;
    _face = face;
    
    // Load static photo from NVS
    if (_storage && _storage->hasCustomPhoto()) {
        _hasPhoto = _storage->loadCustomPhoto(_photo);
    }
    
    // Load video from SPIFFS flash
    _loadVideoFromSpiffs();
}

void AppGallery::onEnter() {
    if (_face) {
        _face->setExpression(Expression::HAPPY, 200);
    }
    
    // If entered via background auto-cycle:
    // User explicitly requested: "ye video cycle me nhi chalna chahiye jaise ki clock face weather photo jaisa"
    if (_isAutoCycle) {
        _isPlayingVideo = false; // Always show static photo during background auto-cycle
    } else {
        // Manual entry: If video exists and no photo, play video. If both exist, show photo with play icon
        if (_hasVideo && !_hasPhoto) {
            _isPlayingVideo = true;
            _videoCurrentFrame = 0;
            _videoLastFrameTime = millis();
        } else {
            _isPlayingVideo = false; // Start on photo, user can tap to play video
        }
    }
}

void AppGallery::onExit() {
    _isPlayingVideo = false;
    _isAutoCycle = false;
}

void AppGallery::update() {
    // Smooth video animation playback
    if (_isPlayingVideo && _hasVideo && _videoFrames && _videoTotalFrames > 0) {
        unsigned long frameInterval = 1000 / (_videoFps > 0 ? _videoFps : 10);
        unsigned long now = millis();
        if (now - _videoLastFrameTime >= frameInterval) {
            _videoCurrentFrame = (_videoCurrentFrame + 1) % _videoTotalFrames;
            _videoLastFrameTime = now;
        }
    }
}

void AppGallery::handleTouch(TouchEvent event) {
    if (event == TouchEvent::TAP) {
        if (_hasVideo) {
            // Single tap toggles between Video playback and Static Photo!
            _isPlayingVideo = !_isPlayingVideo;
            if (_isPlayingVideo) {
                _videoCurrentFrame = 0;
                _videoLastFrameTime = millis();
            }
            if (_face) {
                _face->addHappiness(8);
            }
        } else {
            // Photo only: tap adds affection
            if (_face) {
                _face->addHappiness(5);
            }
        }
    }
}

void AppGallery::setPhoto(const uint8_t* bitmap1024, bool saveToStorage) {
    if (!bitmap1024) return;
    memcpy(_photo, bitmap1024, sizeof(_photo));
    _hasPhoto = true;
    _isPlayingVideo = false; // Show newly uploaded photo
    
    if (saveToStorage && _storage) {
        _storage->saveCustomPhoto(bitmap1024);
        if (_face) {
            _face->setExpression(Expression::PARTY, 400);
        }
    }
}

void AppGallery::clearPhoto() {
    _hasPhoto = false;
    memset(_photo, 0, sizeof(_photo));
    if (_storage) {
        _storage->deleteCustomPhoto();
    }
    if (_hasVideo) {
        _isPlayingVideo = true;
        _videoCurrentFrame = 0;
        _videoLastFrameTime = millis();
    } else {
        _isPlayingVideo = false;
    }
}

bool AppGallery::setVideoStart(uint8_t totalFrames, uint8_t fps) {
    if (totalFrames == 0 || totalFrames > 100) return false;
    
    if (_videoFrames) {
        free(_videoFrames);
        _videoFrames = nullptr;
    }
    
    size_t sz = (size_t)totalFrames * 1024;
    _videoFrames = (uint8_t*)malloc(sz);
    if (!_videoFrames) return false;
    
    memset(_videoFrames, 0, sz);
    _videoTotalFrames = totalFrames;
    _videoFps = (fps > 0 && fps <= 30) ? fps : 10;
    _videoCurrentFrame = 0;
    _hasVideo = false;
    _isPlayingVideo = false;
    return true;
}

bool AppGallery::setVideoFrame(uint8_t index, const uint8_t* bitmap1024) {
    if (!_videoFrames || index >= _videoTotalFrames || !bitmap1024) return false;
    memcpy(_videoFrames + ((size_t)index * 1024), bitmap1024, 1024);
    return true;
}

bool AppGallery::setVideoEnd(bool saveToStorage) {
    if (!_videoFrames || _videoTotalFrames == 0) return false;
    _hasVideo = true;
    _isPlayingVideo = true; // Auto-play immediately after manual video upload!
    _videoCurrentFrame = 0;
    _videoLastFrameTime = millis();
    
    if (saveToStorage) {
        _saveVideoToSpiffs();
    }
    
    if (_face) {
        _face->setExpression(Expression::PARTY, 400);
    }
    return true;
}

void AppGallery::clearVideo() {
    if (_videoFrames) {
        free(_videoFrames);
        _videoFrames = nullptr;
    }
    _hasVideo = false;
    _videoTotalFrames = 0;
    _isPlayingVideo = false;
    
    if (SPIFFS.begin(true)) {
        if (SPIFFS.exists("/buddy_video.bin")) {
            SPIFFS.remove("/buddy_video.bin");
        }
    }
}

void AppGallery::_saveVideoToSpiffs() {
    if (!SPIFFS.begin(true)) return;
    
    File f = SPIFFS.open("/buddy_video.bin", "w");
    if (!f) return;
    
    // Header (8 bytes): 'B','V','I','D', totalFrames, fps, 0, 0
    uint8_t hdr[8] = {'B', 'V', 'I', 'D', _videoTotalFrames, _videoFps, 0, 0};
    f.write(hdr, 8);
    
    if (_videoFrames && _videoTotalFrames > 0) {
        f.write(_videoFrames, (size_t)_videoTotalFrames * 1024);
    }
    f.close();
    Serial.printf("[GALLERY] Saved video to SPIFFS: %d frames @ %d FPS (%d KB)\n", 
                  _videoTotalFrames, _videoFps, (_videoTotalFrames * 1024) / 1024);
}

void AppGallery::_loadVideoFromSpiffs() {
    if (!SPIFFS.begin(true)) return;
    if (!SPIFFS.exists("/buddy_video.bin")) return;
    
    File f = SPIFFS.open("/buddy_video.bin", "r");
    if (!f) return;
    if (f.size() < 8) { f.close(); return; }
    
    uint8_t hdr[8];
    if (f.read(hdr, 8) != 8) { f.close(); return; }
    if (hdr[0] != 'B' || hdr[1] != 'V' || hdr[2] != 'I' || hdr[3] != 'D') { f.close(); return; }
    
    uint8_t totalFrames = hdr[4];
    uint8_t fps = hdr[5];
    if (totalFrames == 0 || totalFrames > 100) { f.close(); return; }
    
    size_t dataLen = (size_t)totalFrames * 1024;
    if (f.size() < 8 + dataLen) { f.close(); return; }
    
    if (_videoFrames) free(_videoFrames);
    _videoFrames = (uint8_t*)malloc(dataLen);
    if (!_videoFrames) { f.close(); return; }
    
    size_t bytesRead = f.read(_videoFrames, dataLen);
    f.close();
    
    if (bytesRead == dataLen) {
        _videoTotalFrames = totalFrames;
        _videoFps = (fps > 0 && fps <= 30) ? fps : 10;
        _hasVideo = true;
        _isPlayingVideo = false; // Loaded video starts paused by default
        Serial.printf("[GALLERY] Loaded video from SPIFFS: %d frames @ %d FPS\n", _videoTotalFrames, _videoFps);
    } else {
        free(_videoFrames);
        _videoFrames = nullptr;
    }
}

void AppGallery::render() {
    if (!_dm) return;
    
    if (!_hasPhoto && !_hasVideo) {
        _renderEmpty();
    } else if (_isPlayingVideo && _hasVideo && _videoFrames) {
        _renderVideo();
    } else {
        _renderPhoto();
    }
}

void AppGallery::_renderEmpty() {
    Adafruit_SSD1306& d = _dm->display();
    
    // Header
    d.setTextSize(1);
    d.setTextColor(SSD1306_WHITE);
    _dm->drawCenteredText("MEDIA STUDIO", 2, 1);
    d.drawLine(0, 12, SCREEN_W, 12, SSD1306_WHITE);
    
    // Subtitle
    d.setCursor(14, 18);
    d.print("Upload from Phone/PC:");
    
    // IP box
    d.drawRoundRect(6, 29, SCREEN_W - 12, 16, 3, SSD1306_WHITE);
    String url = "http://" + (_localIP.length() > 0 ? _localIP : "DeskBuddy-IP") + "/photo";
    _dm->drawCenteredText(url.c_str(), 33, 1);
    
    // Hint
    d.setCursor(8, 50);
    d.print("Photos, Videos & GIFs");
}

void AppGallery::_renderPhoto() {
    Adafruit_SSD1306& d = _dm->display();
    
    // Render full-screen 128x64 bitmap exactly as uploaded
    d.drawBitmap(0, 0, _photo, SCREEN_W, SCREEN_H, SSD1306_WHITE, SSD1306_BLACK);
    
    // If a video is also present, show a small subtle play indicator in bottom right
    if (_hasVideo) {
        d.fillRect(114, 52, 13, 11, SSD1306_BLACK);
        d.drawRoundRect(114, 52, 13, 11, 2, SSD1306_WHITE);
        d.fillTriangle(118, 54, 118, 60, 124, 57, SSD1306_WHITE);
    }
}

void AppGallery::_renderVideo() {
    if (!_dm || !_videoFrames || _videoTotalFrames == 0) return;
    Adafruit_SSD1306& d = _dm->display();
    
    uint8_t* framePtr = _videoFrames + ((size_t)_videoCurrentFrame * 1024);
    d.drawBitmap(0, 0, framePtr, SCREEN_W, SCREEN_H, SSD1306_WHITE, SSD1306_BLACK);
}
