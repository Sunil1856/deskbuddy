// ============================================================================
// DeskBuddy — Settings App Implementation
// ============================================================================
#include "app_settings.h"

void AppSettings::begin(DisplayManager* dm, Storage* storage, WifiManager* wifi, FaceRenderer* face) {
    _dm = dm;
    _storage = storage;
    _wifi = wifi;
    _face = face;
}

void AppSettings::onEnter() {
    _menuIndex = 0;
    if (_storage) {
        _brightness = _storage->getBrightness();
        _is24h = _storage->getTimeFormat24();
        _isFlipped = _storage->getDisplayFlip();
        _eyeStyle = _storage->getEyeStyle();
        _sleepTimeout = _storage->getSleepTimeout();
        _autoCycle = _storage->getAutoCycle();
    }
}

void AppSettings::onExit() {
}

void AppSettings::update() {
}

void AppSettings::handleTouch(TouchEvent event) {
    if (event == TouchEvent::TAP) {
        _menuIndex = (_menuIndex + 1) % NUM_SETTINGS;
    } else if (event == TouchEvent::LONG_PRESS) {
        // Toggle/Change value
        switch (_menuIndex) {
            case 0: // Brightness
                _brightness = (_brightness + 1) % 4;
                if (_dm) _dm->setBrightness(_brightness);
                if (_storage) _storage->setBrightness(_brightness);
                break;
                
            case 1: // Time Format
                _is24h = !_is24h;
                if (_storage) _storage->setTimeFormat24(_is24h);
                break;
                
            case 2: // Display Flip
                _isFlipped = !_isFlipped;
                if (_dm) _dm->setFlip(_isFlipped);
                if (_storage) _storage->setDisplayFlip(_isFlipped);
                break;
                
            case 3: // Eye Style
                _eyeStyle = (_eyeStyle + 1) % (uint8_t)EyeStyle::COUNT;
                if (_face) _face->setEyeStyle((EyeStyle)_eyeStyle);
                if (_storage) _storage->setEyeStyle(_eyeStyle);
                break;
                
            case 4: // Auto Cycle
                _autoCycle = (_autoCycle + 1) % 5;
                if (_storage) _storage->setAutoCycle(_autoCycle);
                break;
                
            case 5: // Expression Preview
                _previewExprIndex = (_previewExprIndex + 1) % (int)Expression::COUNT;
                if (_face) _face->setExpression((Expression)_previewExprIndex, 250);
                break;
                
            case 6: // Pet Mood (Petting boost)
                if (_face) _face->setHappiness(100);
                break;
                
            case 7: // Sleep Timeout (5m, 10m, 15m, 30m, OFF)
                if (_sleepTimeout == 300000) _sleepTimeout = 600000;       // 5m -> 10m
                else if (_sleepTimeout == 600000) _sleepTimeout = 900000;  // 10m -> 15m
                else if (_sleepTimeout == 900000) _sleepTimeout = 1800000; // 15m -> 30m
                else if (_sleepTimeout == 1800000) _sleepTimeout = 0;      // 30m -> OFF
                else _sleepTimeout = 300000;                               // OFF / other -> 5m
                if (_storage) _storage->setSleepTimeout(_sleepTimeout);
                break;
                
            case 8: // WiFi Reset
                if (_storage) {
                    _storage->setConfigured(false);
                    _storage->setWifi("", "");
                }
                if (_wifi) _wifi->disconnect();
                delay(300);
                ESP.restart();
                break;
                
            case 9: // Erase All
                if (_storage) {
                    _storage->eraseAll();
                    _storage->setConfigured(false);
                }
                delay(300);
                ESP.restart();
                break;
        }
    }
}

void AppSettings::_drawSettingRow(int y, const char* label, const char* valStr, bool selected) {
    Adafruit_SSD1306& d = _dm->display();
    
    if (selected) {
        d.fillRect(0, y - 1, SCREEN_W, 10, SSD1306_WHITE);
        d.setTextColor(SSD1306_BLACK);
    }
    
    d.setCursor(2, y);
    d.print(label);
    
    int16_t x1, y1;
    uint16_t w, h;
    d.getTextBounds(valStr, 0, 0, &x1, &y1, &w, &h);
    d.setCursor(SCREEN_W - w - 2, y);
    d.print(valStr);
    
    if (selected) {
        d.setTextColor(SSD1306_WHITE);
    }
}

void AppSettings::render() {
    if (!_dm) return;
    Adafruit_SSD1306& d = _dm->display();
    
    d.setTextSize(1);
    _dm->drawCenteredText("SETTINGS", 2, 1);
    d.drawLine(0, 12, SCREEN_W, 12, SSD1306_WHITE);
    
    // Calculate scroll window
    int startIdx = _menuIndex - 2;
    if (startIdx < 0) startIdx = 0;
    if (startIdx > NUM_SETTINGS - 4) startIdx = NUM_SETTINGS - 4;
    
    int yBase = 16;
    int yStep = 11;
    
    char buf[24];
    
    for (int i = 0; i < 4; i++) {
        int idx = startIdx + i;
        if (idx >= NUM_SETTINGS) break;
        
        const char* label = "";
        const char* val = "";
        
        switch (idx) {
            case 0: 
                label = "Brightness"; 
                val = _brightness == 3 ? "100%" : (_brightness == 2 ? "75%" : (_brightness == 1 ? "50%" : "25%"));
                break;
            case 1:
                label = "Time Fmt";
                val = _is24h ? "24h" : "12h";
                break;
            case 2:
                label = "Screen Flip";
                val = _isFlipped ? "ON" : "OFF";
                break;
            case 3:
                label = "Eye Style";
                val = _eyeStyle == 0 ? "Classic" : (_eyeStyle == 1 ? "Vector" : (_eyeStyle == 2 ? "Neon" : "Cyber"));
                break;
            case 4:
                label = "Auto Cycle";
                val = _autoCycle == 0 ? "OFF" : (_autoCycle == 1 ? "1 min" : (_autoCycle == 2 ? "2 min" : (_autoCycle == 3 ? "3 min" : "5 min")));
                break;
            case 5:
                label = "Preview Face";
                if (_face) {
                    snprintf(buf, sizeof(buf), "%d/30 %s", _previewExprIndex + 1, _face->getExpressionName((Expression)_previewExprIndex));
                } else {
                    snprintf(buf, sizeof(buf), "%d/30", _previewExprIndex + 1);
                }
                val = buf;
                break;
            case 6:
                label = "Pet Mood";
                if (_face) {
                    snprintf(buf, sizeof(buf), "%d%% Happy", _face->getHappiness());
                } else {
                    snprintf(buf, sizeof(buf), "75%%");
                }
                val = buf;
                break;
            case 7:
                label = "Sleep in";
                if (_sleepTimeout == 0) {
                    val = "OFF";
                } else {
                    snprintf(buf, sizeof(buf), "%d min", (int)(_sleepTimeout / 60000));
                    val = buf;
                }
                break;
            case 8:
                label = "Reset WiFi";
                val = "Hold";
                break;
            case 9:
                label = "Factory Rst";
                val = "Hold";
                break;
        }
        
        _drawSettingRow(yBase + i * yStep, label, val, idx == _menuIndex);
    }
    
    // IP / version footer
    d.setTextSize(1);
    snprintf(buf, sizeof(buf), "IP: %s", _wifi ? _wifi->getIP().c_str() : "...");
    _dm->drawCenteredText(buf, 60 - 4, 1);
}
