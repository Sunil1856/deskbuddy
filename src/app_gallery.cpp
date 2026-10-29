// ============================================================================
// DeskBuddy — Photo Gallery / Live Photo Frame Implementation
// ============================================================================
#include "app_gallery.h"
#include "icons.h"

AppGallery::AppGallery() {
    memset(_photo, 0, sizeof(_photo));
}

void AppGallery::begin(DisplayManager* dm, Storage* storage, FaceRenderer* face) {
    _dm = dm;
    _storage = storage;
    _face = face;
    
    if (_storage && _storage->hasCustomPhoto()) {
        _hasPhoto = _storage->loadCustomPhoto(_photo);
    }
}

void AppGallery::onEnter() {
    if (_face) {
        _face->setExpression(Expression::HAPPY, 200);
    }
}

void AppGallery::onExit() {
}

void AppGallery::update() {
}

void AppGallery::handleTouch(TouchEvent event) {
    // Invert option removed as requested: image is always shown exactly as uploaded
}

void AppGallery::setPhoto(const uint8_t* bitmap1024, bool saveToStorage) {
    if (!bitmap1024) return;
    memcpy(_photo, bitmap1024, sizeof(_photo));
    _hasPhoto = true;
    if (saveToStorage && _storage) {
        _storage->saveCustomPhoto(bitmap1024);
        if (_face) {
            _face->setExpression(Expression::PARTY, 400);
        }
    }
}

void AppGallery::render() {
    if (!_dm) return;
    
    if (!_hasPhoto) {
        _renderEmpty();
    } else {
        _renderPhoto();
    }
}

void AppGallery::_renderEmpty() {
    Adafruit_SSD1306& d = _dm->display();
    
    // Header
    d.setTextSize(1);
    d.setTextColor(SSD1306_WHITE);
    _dm->drawCenteredText("PHOTO STUDIO", 2, 1);
    d.drawLine(0, 12, SCREEN_W, 12, SSD1306_WHITE);
    
    // Subtitle
    d.setCursor(14, 18);
    d.print("Upload from Phone/PC:");
    
    // IP box
    d.drawRoundRect(6, 29, SCREEN_W - 12, 16, 3, SSD1306_WHITE);
    String url = "http://" + (_localIP.length() > 0 ? _localIP : "DeskBuddy-IP") + "/photo";
    _dm->drawCenteredText(url.c_str(), 33, 1);
    
    // Hint
    d.setCursor(16, 50);
    d.print("Live 128x64 Canvas");
}

void AppGallery::_renderPhoto() {
    Adafruit_SSD1306& d = _dm->display();
    
    // Render full-screen 128x64 bitmap exactly as uploaded (pure 1:1 pixel fidelity)
    d.drawBitmap(0, 0, _photo, SCREEN_W, SCREEN_H, SSD1306_WHITE, SSD1306_BLACK);
}
