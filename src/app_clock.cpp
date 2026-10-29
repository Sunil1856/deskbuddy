// ============================================================================
// DeskBuddy — Clock App Implementation
// ============================================================================
#include "app_clock.h"
#include "fonts.h"
#include "icons.h"
#include "easing.h"
#include <math.h>

void AppClock::begin(DisplayManager* dm, TimeService* timeSvc, Storage* storage) {
    _dm = dm;
    _time = timeSvc;
    _storage = storage;
}

void AppClock::onEnter() {
    _mode = ClockMode::MODE_DIGITAL; // Always default to Cyber G-Shock!
    _digitalStyle = 0; // Style 0: Cyber 7-Segment
    if (_storage) {
        _storage->setClockMode(0);
        _storage->saveInt("clk_style", 0);
    }
}

void AppClock::onExit() {
    // nothing to clean up
}

void AppClock::update() {
    unsigned long now = millis();
    
    if (_mode == ClockMode::MODE_STOPWATCH && _swRunning) {
        _swElapsed = now - _swStart;
    }
    else if (_mode == ClockMode::MODE_TIMER && _tmrRunning) {
        if (now - _tmrStart >= _tmrDuration) {
            _tmrRunning = false;
            _tmrRemaining = 0;
        } else {
            _tmrRemaining = _tmrDuration - (now - _tmrStart);
        }
    }
}

void AppClock::handleTouch(TouchEvent event) {
    switch (event) {
        case TouchEvent::TAP:
            if (_mode == ClockMode::MODE_STOPWATCH) {
                // Start / Stop stopwatch
                if (_swRunning) {
                    _swRunning = false;
                } else {
                    if (_swElapsed == 0) {
                        _swStart = millis();
                    } else {
                        // Resume
                        _swStart = millis() - _swElapsed;
                    }
                    _swRunning = true;
                }
            } else if (_mode == ClockMode::MODE_TIMER && !_tmrRunning) {
                // Cycle timer presets (1m, 3m, 5m, 10m, 15m, 25m Pomodoro, 60m)
                _tmrPresetIdx = (_tmrPresetIdx + 1) % 7;
                _tmrRemaining = _tmrPresets[_tmrPresetIdx] * 60000UL;
            } else if (_mode == ClockMode::MODE_TIMER && _tmrRunning) {
                // Stop timer
                _tmrRunning = false;
            } else {
                // Cycle clock modes: DIGITAL -> ANALOG -> BINARY -> STOPWATCH -> TIMER -> DIGITAL
                _mode = (ClockMode)(((int)_mode + 1) % (int)ClockMode::COUNT);
                if (_storage && _mode < ClockMode::MODE_STOPWATCH) {
                    _storage->setClockMode((uint8_t)_mode);
                }
                // Reset states
                _swRunning = false;
                _swElapsed = 0;
                _tmrRunning = false;
                _tmrPresetIdx = 0;
                _tmrRemaining = _tmrPresets[0] * 60000UL;
            }
            break;
            
        case TouchEvent::LONG_PRESS:
            if (_mode == ClockMode::MODE_STOPWATCH) {
                // Long press exits stopwatch and cycles to Timer!
                _swRunning = false;
                _swElapsed = 0;
                _mode = ClockMode::MODE_TIMER;
            } else if (_mode == ClockMode::MODE_TIMER) {
                if (!_tmrRunning && _tmrRemaining > 0) {
                    // Start timer
                    _tmrRunning = true;
                    _tmrDuration = _tmrRemaining;
                    _tmrStart = millis();
                } else {
                    // Reset and return to Jumbo Digital Clock
                    _tmrRunning = false;
                    _mode = ClockMode::MODE_DIGITAL;
                    if (_storage) _storage->setClockMode(0);
                }
            } else if (_mode != ClockMode::MODE_DIGITAL) {
                // On Analog/Binary: Long press resets to Digital Clock (default)
                _mode = ClockMode::MODE_DIGITAL;
                if (_storage) _storage->setClockMode(0);
            }
            break;
            
        default:
            break;
    }
}

void AppClock::render() {
    if (!_dm || !_time) return;
    
    switch (_mode) {
        case ClockMode::MODE_DIGITAL:    _renderDigital(); break;
        case ClockMode::MODE_ANALOG:     _renderAnalog(); break;
        case ClockMode::MODE_BINARY:     _renderBinary(); break;
        case ClockMode::MODE_STOPWATCH:  _renderStopwatch(); break;
        case ClockMode::MODE_TIMER:      _renderTimer(); break;
        default: break;
    }
}

void AppClock::_draw7Segment(int x, int y, int w, int h, int num) {
    if (num < 0 || num > 9) return;
    Adafruit_SSD1306& d = _dm->display();
    
    // Segment bitmask: bit 0=a, 1=b, 2=c, 3=d, 4=e, 5=f, 6=g
    static const uint8_t DIGIT_MAP[10] = {
        0x3F, // 0: a b c d e f
        0x06, // 1: b c
        0x5B, // 2: a b d e g
        0x4F, // 3: a b c d g
        0x66, // 4: b c f g
        0x6D, // 5: a c d f g
        0x7D, // 6: a c d e f g
        0x07, // 7: a b c
        0x7F, // 8: a b c d e f g
        0x6F  // 9: a b c d f g
    };
    
    uint8_t mask = DIGIT_MAP[num];
    const int t = 3; // segment thickness
    const int midY = y + (h / 2);
    
    // 100% Crisp High-Contrast OLED - Only lit segments are drawn, unlit is pure pitch black!
    // a: top horizontal
    if (mask & 0x01) d.fillRect(x + 2, y, w - 4, t, SSD1306_WHITE);
    // b: top-right vertical
    if (mask & 0x02) d.fillRect(x + w - t, y + 2, t, (h / 2) - 3, SSD1306_WHITE);
    // c: bottom-right vertical
    if (mask & 0x04) d.fillRect(x + w - t, midY + 1, t, (h / 2) - 3, SSD1306_WHITE);
    // d: bottom horizontal
    if (mask & 0x08) d.fillRect(x + 2, y + h - t, w - 4, t, SSD1306_WHITE);
    // e: bottom-left vertical
    if (mask & 0x10) d.fillRect(x, midY + 1, t, (h / 2) - 3, SSD1306_WHITE);
    // f: top-left vertical
    if (mask & 0x20) d.fillRect(x, y + 2, t, (h / 2) - 3, SSD1306_WHITE);
    // g: middle horizontal
    if (mask & 0x40) d.fillRect(x + 2, midY - 1, w - 4, t, SSD1306_WHITE);
}

void AppClock::_renderDigital() {
    _renderDigitalCyber();
}

void AppClock::_renderDigitalCyber() {
    Adafruit_SSD1306& d = _dm->display();
    d.setFont();
    
    bool is24 = _storage ? _storage->getTimeFormat24() : true;
    
    int h = _time->getHour();
    int m = _time->getMinute();
    int s = _time->getSecond();
    
    bool pm = false;
    if (!is24) {
        pm = (h >= 12);
        if (h == 0) h = 12;
        else if (h > 12) h -= 12;
    }
    
    // --- 0. CYBER G-SHOCK BEZEL / OUTER FRAME ---
    d.drawRoundRect(0, 0, SCREEN_W, SCREEN_H, 2, SSD1306_WHITE);
    
    // --- 1. TOP HEADER BAR ---
    String rawDate = _time->getFormattedDate(); // e.g. "WED, OCT 28"
    String dayStr = "WED";
    String dateStr = rawDate;
    int comma = rawDate.indexOf(',');
    if (comma > 0) {
        dayStr = rawDate.substring(0, comma);
        dayStr.trim();
        dateStr = rawDate.substring(comma + 1);
        dateStr.trim();
    }
    
    // Inverted Day Pill badge
    d.fillRoundRect(2, 2, 24, 9, 1, SSD1306_WHITE);
    d.setTextColor(SSD1306_BLACK);
    d.setTextSize(1);
    d.setCursor(4, 3);
    d.print(dayStr);
    
    // Date & Year
    d.setTextColor(SSD1306_WHITE);
    char dateBuf[20];
    int yr = _time->getYear();
    if (yr > 2000) {
        snprintf(dateBuf, sizeof(dateBuf), "%s '%02d", dateStr.c_str(), yr % 100);
    } else {
        snprintf(dateBuf, sizeof(dateBuf), "%s", dateStr.c_str());
    }
    d.setCursor(32, 3);
    d.print(dateBuf);
    
    // Live Sync beacon on top-right (square dot)
    if (_time->isSynced()) {
        d.fillRect(121, 4, 3, 3, SSD1306_WHITE);
    } else {
        d.drawRect(121, 4, 3, 3, SSD1306_WHITE);
    }
    
    // Header divider line
    d.drawFastHLine(0, 13, SCREEN_W, SSD1306_WHITE);
    
    // --- 2. MAIN 7-SEGMENT TIME (100% Crisp High-Contrast OLED - Zero Ghost Blur) ---
    const int digW = 16;
    const int digH = 30;
    const int digY = 16;
    
    // Hours
    _draw7Segment(8, digY, digW, digH, h / 10);
    _draw7Segment(27, digY, digW, digH, h % 10);
    
    // Solid G-Shock Square Colon
    if (s % 2 == 0) {
        d.fillRect(48, digY + 8, 3, 3, SSD1306_WHITE);
        d.fillRect(48, digY + 18, 3, 3, SSD1306_WHITE);
    } else {
        d.drawRect(48, digY + 8, 3, 3, SSD1306_WHITE);
        d.drawRect(48, digY + 18, 3, 3, SSD1306_WHITE);
    }
    
    // Minutes
    _draw7Segment(55, digY, digW, digH, m / 10);
    _draw7Segment(74, digY, digW, digH, m % 10);
    
    // --- 3. RIGHT WING (AM/PM & SECONDS) ---
    // AM/PM or 24H Badge
    d.setTextSize(1);
    if (!is24) {
        if (pm) {
            d.fillRoundRect(96, 17, 28, 11, 2, SSD1306_WHITE);
            d.setTextColor(SSD1306_BLACK);
            d.setCursor(103, 19);
            d.print("PM");
        } else {
            d.drawRoundRect(96, 17, 28, 11, 2, SSD1306_WHITE);
            d.setTextColor(SSD1306_WHITE);
            d.setCursor(103, 19);
            d.print("AM");
        }
    } else {
        d.drawRoundRect(96, 17, 28, 11, 2, SSD1306_WHITE);
        d.setTextColor(SSD1306_WHITE);
        d.setCursor(100, 19);
        d.print("24H");
    }
    
    // Running Seconds Box
    d.drawRoundRect(96, 31, 28, 15, 2, SSD1306_WHITE);
    d.setTextColor(SSD1306_WHITE);
    char secStr[6];
    snprintf(secStr, sizeof(secStr), ":%02d", s);
    d.setCursor(100, 35);
    d.print(secStr);
    
    // --- 4. FOOTER: 60-SECOND GRADUATED TRACK ---
    d.drawFastHLine(0, 49, SCREEN_W, SSD1306_WHITE);
    
    // Calibration tick marks at 15s, 30s, 45s connecting line to meter
    d.fillRect(33, 50, 1, 3, SSD1306_WHITE);
    d.fillRect(64, 50, 1, 3, SSD1306_WHITE);
    d.fillRect(95, 50, 1, 3, SSD1306_WHITE);
    
    // Track frame (124px wide)
    d.drawRoundRect(2, 53, 124, 8, 2, SSD1306_WHITE);
    
    // Fill progress
    int fw = (s * 120) / 59;
    if (fw > 0) {
        d.fillRect(4, 55, fw, 4, SSD1306_WHITE);
    }
}

void AppClock::_drawModernDigit(int x, int y, int w, int h, int num) {
    if (num < 0 || num > 9) return;
    Adafruit_SSD1306& d = _dm->display();
    const int midY = y + (h / 2);

    switch (num) {
        case 0:
            d.fillRoundRect(x, y, w, h, 3, SSD1306_WHITE);
            d.fillRoundRect(x + 3, y + 3, w - 6, h - 6, 2, SSD1306_BLACK);
            break;
        case 1:
            d.fillRect(x + w - 4, y, 3, h, SSD1306_WHITE);
            d.drawLine(x + w - 7, y + 3, x + w - 4, y, SSD1306_WHITE);
            d.fillRect(x + 4, y + h - 3, w - 4, 3, SSD1306_WHITE);
            break;
        case 2:
            d.fillRect(x, y, w, 3, SSD1306_WHITE);
            d.fillRect(x + w - 3, y, 3, h/2, SSD1306_WHITE);
            d.fillRect(x, midY - 1, w, 3, SSD1306_WHITE);
            d.fillRect(x, midY - 1, 3, h/2, SSD1306_WHITE);
            d.fillRect(x, y + h - 3, w, 3, SSD1306_WHITE);
            break;
        case 3:
            d.fillRect(x, y, w, 3, SSD1306_WHITE);
            d.fillRect(x + w - 3, y, 3, h, SSD1306_WHITE);
            d.fillRect(x + 3, midY - 1, w - 3, 3, SSD1306_WHITE);
            d.fillRect(x, y + h - 3, w, 3, SSD1306_WHITE);
            break;
        case 4:
            d.fillRect(x, y, 3, h/2 + 1, SSD1306_WHITE);
            d.fillRect(x, midY - 1, w, 3, SSD1306_WHITE);
            d.fillRect(x + w - 4, y, 3, h, SSD1306_WHITE);
            break;
        case 5:
            d.fillRect(x, y, w, 3, SSD1306_WHITE);
            d.fillRect(x, y, 3, h/2, SSD1306_WHITE);
            d.fillRect(x, midY - 1, w, 3, SSD1306_WHITE);
            d.fillRect(x + w - 3, midY - 1, 3, h/2, SSD1306_WHITE);
            d.fillRect(x, y + h - 3, w, 3, SSD1306_WHITE);
            break;
        case 6:
            d.fillRect(x, y, 3, h, SSD1306_WHITE);
            d.fillRect(x, y, w, 3, SSD1306_WHITE);
            d.fillRect(x, midY - 1, w, 3, SSD1306_WHITE);
            d.fillRect(x + w - 3, midY - 1, 3, h/2, SSD1306_WHITE);
            d.fillRect(x, y + h - 3, w, 3, SSD1306_WHITE);
            break;
        case 7:
            d.fillRect(x, y, w, 3, SSD1306_WHITE);
            d.fillRect(x + w - 3, y, 3, h, SSD1306_WHITE);
            break;
        case 8:
            d.fillRoundRect(x, y, w, h, 3, SSD1306_WHITE);
            d.fillRoundRect(x + 3, y + 3, w - 6, (h/2) - 4, 1, SSD1306_BLACK);
            d.fillRoundRect(x + 3, midY + 1, w - 6, (h/2) - 4, 1, SSD1306_BLACK);
            break;
        case 9:
            d.fillRect(x, y, w, 3, SSD1306_WHITE);
            d.fillRect(x, y, 3, h/2, SSD1306_WHITE);
            d.fillRect(x + w - 3, y, 3, h, SSD1306_WHITE);
            d.fillRect(x, midY - 1, w, 3, SSD1306_WHITE);
            d.fillRect(x, y + h - 3, w, 3, SSD1306_WHITE);
            break;
    }
}

void AppClock::_renderDigitalModern() {
    Adafruit_SSD1306& d = _dm->display();
    d.setFont();
    
    bool is24 = _storage ? _storage->getTimeFormat24() : true;
    int h = _time->getHour();
    int m = _time->getMinute();
    int s = _time->getSecond();
    
    bool pm = false;
    if (!is24) {
        pm = (h >= 12);
        if (h == 0) h = 12;
        else if (h > 12) h -= 12;
    }
    
    // Top Date Bar
    d.setTextSize(1);
    d.setTextColor(SSD1306_WHITE);
    _dm->drawCenteredText(_time->getFormattedDate().c_str(), 2, 1);
    d.drawFastHLine(4, 12, SCREEN_W - 8, SSD1306_WHITE);
    
    // Big Sleek Vector Digits (100% bug-free, crisp solid strokes)
    const int digW = 16;
    const int digH = 30;
    const int digY = 16;
    
    // Hours
    _drawModernDigit(8, digY, digW, digH, h / 10);
    _drawModernDigit(27, digY, digW, digH, h % 10);
    
    // Pulsing Colon
    if (s % 2 == 0) {
        d.fillCircle(49, digY + 9, 2, SSD1306_WHITE);
        d.fillCircle(49, digY + 21, 2, SSD1306_WHITE);
    } else {
        d.drawCircle(49, digY + 9, 2, SSD1306_WHITE);
        d.drawCircle(49, digY + 21, 2, SSD1306_WHITE);
    }
    
    // Minutes
    _drawModernDigit(55, digY, digW, digH, m / 10);
    _drawModernDigit(74, digY, digW, digH, m % 10);
    
    // Reset font for badges
    d.setTextSize(1);
    
    // Stacked AM/PM & Seconds on right
    if (!is24) {
        d.drawRoundRect(96, 17, 28, 11, 2, SSD1306_WHITE);
        d.setCursor(102, 19);
        d.print(pm ? "PM" : "AM");
    } else {
        d.drawRoundRect(96, 17, 28, 11, 2, SSD1306_WHITE);
        d.setCursor(102, 19);
        d.print("24H");
    }
    
    // Running seconds box
    d.drawRoundRect(96, 31, 28, 15, 2, SSD1306_WHITE);
    char secBuf[6];
    snprintf(secBuf, sizeof(secBuf), ":%02d", s);
    d.setCursor(100, 35);
    d.print(secBuf);
    
    // Modern bottom progress line
    d.drawFastHLine(0, 49, SCREEN_W, SSD1306_WHITE);
    d.drawRoundRect(2, 53, 124, 8, 2, SSD1306_WHITE);
    int secW = (s * 120) / 59;
    if (secW > 0) d.fillRect(4, 55, secW, 4, SSD1306_WHITE);
}

void AppClock::_renderAnalog() {
    Adafruit_SSD1306& d = _dm->display();
    
    int cx = SCREEN_W / 2; // 64
    int cy = 32;
    int r = 30;
    
    // Dial outer circle
    d.drawCircle(cx, cy, r, SSD1306_WHITE);
    
    // Dial numbers: 12, 3, 6, 9
    d.setFont();
    d.setTextSize(1);
    d.setTextColor(SSD1306_WHITE);
    
    // "12" at top
    d.setCursor(cx - 5, cy - r + 3);
    d.print("12");
    
    // "6" at bottom
    d.setCursor(cx - 2, cy + r - 10);
    d.print("6");
    
    // "3" at right
    d.setCursor(cx + r - 9, cy - 3);
    d.print("3");
    
    // "9" at left
    d.setCursor(cx - r + 4, cy - 3);
    d.print("9");
    
    // Tick marks for remaining hours (1, 2, 4, 5, 7, 8, 10, 11)
    for (int i = 0; i < 12; i++) {
        if (i == 0 || i == 3 || i == 6 || i == 9) continue;
        float angle = i * 30.0f * (M_PI / 180.0f);
        int px1 = cx + sinf(angle) * (r - 3);
        int py1 = cy - cosf(angle) * (r - 3);
        int px2 = cx + sinf(angle) * r;
        int py2 = cy - cosf(angle) * r;
        d.drawLine(px1, py1, px2, py2, SSD1306_WHITE);
    }
    
    int h = _time->getHour();
    int m = _time->getMinute();
    int s = _time->getSecond();
    
    // Angles
    float hAngle = (h % 12 + m / 60.0f) * 30.0f * (M_PI / 180.0f);
    float mAngle = (m + s / 60.0f) * 6.0f * (M_PI / 180.0f);
    float sAngle = s * 6.0f * (M_PI / 180.0f);
    
    // Hands: Hour (thick), Minute (thick), Second (thin)
    _drawHand(cx, cy, hAngle, r * 0.45f, true);
    _drawHand(cx, cy, mAngle, r * 0.72f, true);
    _drawHand(cx, cy, sAngle, r * 0.88f, false);
    
    // Center cap dot
    d.fillCircle(cx, cy, 2, SSD1306_WHITE);
    
    // Top-left digital time & Top-right Day badge
    bool is24 = _storage ? _storage->getTimeFormat24() : true;
    String miniTime = _time->getFormattedTime(is24 ? false : true, false);
    d.setCursor(2, 2);
    d.print(miniTime);
    
    String dateStr = _time->getFormattedDate();
    int comma = dateStr.indexOf(',');
    if (comma > 0) dateStr = dateStr.substring(0, comma);
    d.setCursor(SCREEN_W - 22, 2);
    d.print(dateStr);
}

void AppClock::_drawHand(int cx, int cy, float angle, int length, bool thick) {
    Adafruit_SSD1306& d = _dm->display();
    int px = cx + sinf(angle) * length;
    int py = cy - cosf(angle) * length;
    
    d.drawLine(cx, cy, px, py, SSD1306_WHITE);
    if (thick) {
        int ox = cosf(angle) * 1;
        int oy = sinf(angle) * 1;
        d.drawLine(cx + ox, cy + oy, px + ox, py + oy, SSD1306_WHITE);
        d.drawLine(cx - ox, cy - oy, px - ox, py - oy, SSD1306_WHITE);
    }
}

void AppClock::_renderBinary() {
    Adafruit_SSD1306& d = _dm->display();
    
    int h = _time->getHour();
    int m = _time->getMinute();
    int s = _time->getSecond();
    
    // H, M, S as BCD
    int h10 = h / 10, h1 = h % 10;
    int m10 = m / 10, m1 = m % 10;
    int s10 = s / 10, s1 = s % 10;
    
    int vals[6] = {h10, h1, m10, m1, s10, s1};
    
    int startX = 20;
    int gapX = 16;
    int gapY = 12;
    int startY = 50;
    
    d.setFont();
    d.setTextSize(1);
    
    // Header labels
    d.setCursor(startX + gapX/2 - 2, 4); d.print("H");
    d.setCursor(startX + 2*gapX + gapX/2 - 2, 4); d.print("M");
    d.setCursor(startX + 4*gapX + gapX/2 - 2, 4); d.print("S");
    
    for (int col = 0; col < 6; col++) {
        for (int row = 0; row < 4; row++) {
            if (col == 0 && row > 1) continue;
            if ((col == 2 || col == 4) && row > 2) continue;
            
            int bx = startX + col * gapX;
            int by = startY - row * gapY;
            
            bool isOn = (vals[col] & (1 << row)) != 0;
            
            if (isOn) {
                d.fillCircle(bx, by, 4, SSD1306_WHITE);
            } else {
                d.drawCircle(bx, by, 4, SSD1306_WHITE);
            }
        }
    }
}

void AppClock::_renderStopwatch() {
    Adafruit_SSD1306& d = _dm->display();
    d.setFont();
    
    unsigned long ms = (_swElapsed % 1000) / 10; // centiseconds (00-99)
    unsigned long sec = (_swElapsed / 1000) % 60;
    unsigned long min = (_swElapsed / 60000) % 60;
    
    // --- 1. TOP HEADER BAR ---
    d.fillRoundRect(2, 1, 62, 10, 2, SSD1306_WHITE);
    d.setTextColor(SSD1306_BLACK);
    d.setTextSize(1);
    d.setCursor(5, 2);
    d.print("STOPWATCH");
    
    // Status
    d.setTextColor(SSD1306_WHITE);
    if (_swRunning) {
        d.setCursor(SCREEN_W - 44, 2);
        d.print("RUN");
        if ((millis() / 500) % 2 == 0) {
            d.fillCircle(SCREEN_W - 6, 5, 2, SSD1306_WHITE);
        } else {
            d.drawCircle(SCREEN_W - 6, 5, 2, SSD1306_WHITE);
        }
    } else {
        d.setCursor(SCREEN_W - 44, 2);
        d.print(_swElapsed > 0 ? "PAUSE" : "READY");
    }
    
    d.drawFastHLine(0, 13, SCREEN_W, SSD1306_WHITE);
    
    // --- 2. MAIN DIGITS: MM:SS with stacked centiseconds ---
    const int digW = 14;
    const int digH = 26;
    const int digY = 16;
    
    int m1 = min / 10;
    int m2 = min % 10;
    int s1 = sec / 10;
    int s2 = sec % 10;
    
    if (_digitalStyle == 0) {
        _draw7Segment(14, digY, digW, digH, m1);
        _draw7Segment(31, digY, digW, digH, m2);
        
        // Square Colon
        d.fillRect(47, digY + 8, 3, 3, SSD1306_WHITE);
        d.fillRect(47, digY + 18, 3, 3, SSD1306_WHITE);
        
        _draw7Segment(54, digY, digW, digH, s1);
        _draw7Segment(71, digY, digW, digH, s2);
    } else {
        _drawModernDigit(14, digY, digW, digH, m1);
        _drawModernDigit(31, digY, digW, digH, m2);
        
        // Colon
        d.fillCircle(48, digY + 8, 2, SSD1306_WHITE);
        d.fillCircle(48, digY + 18, 2, SSD1306_WHITE);
        
        _drawModernDigit(54, digY, digW, digH, s1);
        _drawModernDigit(71, digY, digW, digH, s2);
    }
    
    // Centiseconds Box on right (.CS)
    d.drawRoundRect(90, 17, 34, 24, 2, SSD1306_WHITE);
    d.setTextSize(1);
    d.setTextColor(SSD1306_WHITE);
    d.setCursor(95, 20);
    d.print(".CS");
    char msBuf[6];
    snprintf(msBuf, sizeof(msBuf), "%02lu", ms);
    d.setCursor(95, 30);
    d.print(msBuf);
    
    // --- 3. BOTTOM 60s RUNNER TRACK ---
    d.drawFastHLine(0, 46, SCREEN_W, SSD1306_WHITE);
    
    d.drawRoundRect(2, 49, 124, 6, 2, SSD1306_WHITE);
    int secBar = (int)((sec * 120) / 59);
    if (secBar > 0) d.fillRect(4, 51, secBar, 2, SSD1306_WHITE);
    
    d.setTextSize(1);
    d.setCursor(4, 56);
    if (_swRunning) {
        d.print("Tap:Pause  | Hold:Timer");
    } else {
        d.print(_swElapsed == 0 ? "Tap:Start  | Hold:Timer" : "Tap:Resume | Hold:Timer");
    }
}

void AppClock::_renderTimer() {
    Adafruit_SSD1306& d = _dm->display();
    d.setFont();
    
    unsigned long sec = (_tmrRemaining / 1000) % 60;
    unsigned long min = (_tmrRemaining / 60000);
    
    // --- 1. TOP HEADER BAR ---
    int presetMin = _tmrPresets[_tmrPresetIdx];
    char presetBuf[16];
    if (presetMin == 25) {
        snprintf(presetBuf, sizeof(presetBuf), "25m POMO");
    } else {
        snprintf(presetBuf, sizeof(presetBuf), "TMR %02dm", presetMin);
    }
    
    d.fillRoundRect(2, 1, 58, 10, 2, SSD1306_WHITE);
    d.setTextColor(SSD1306_BLACK);
    d.setTextSize(1);
    d.setCursor(5, 2);
    d.print(presetBuf);
    
    // Right: Status Badge
    d.setTextColor(SSD1306_WHITE);
    if (_tmrRunning) {
        d.setCursor(SCREEN_W - 44, 2);
        d.print("RUN");
        // Pulsing run dot
        if ((millis() / 500) % 2 == 0) {
            d.fillCircle(SCREEN_W - 6, 5, 2, SSD1306_WHITE);
        } else {
            d.drawCircle(SCREEN_W - 6, 5, 2, SSD1306_WHITE);
        }
    } else {
        d.setCursor(SCREEN_W - 48, 2);
        d.print(_tmrRemaining > 0 ? "PAUSE" : "DONE!");
    }
    
    d.drawFastHLine(0, 13, SCREEN_W, SSD1306_WHITE);
    
    // --- 2. MAIN COUNTDOWN DIGITS (26px High Vector Math) ---
    const int digW = 14;
    const int digH = 26;
    const int digY = 16;
    
    int m1 = min / 10;
    int m2 = min % 10;
    int s1 = sec / 10;
    int s2 = sec % 10;
    
    if (_digitalStyle == 0) {
        _draw7Segment(24, digY, digW, digH, m1);
        _draw7Segment(41, digY, digW, digH, m2);
        
        // Pulsing Square Colon
        if (!_tmrRunning || (millis() / 500) % 2 == 0) {
            d.fillRect(57, digY + 8, 3, 3, SSD1306_WHITE);
            d.fillRect(57, digY + 18, 3, 3, SSD1306_WHITE);
        }
        
        _draw7Segment(68, digY, digW, digH, s1);
        _draw7Segment(85, digY, digW, digH, s2);
    } else {
        _drawModernDigit(24, digY, digW, digH, m1);
        _drawModernDigit(41, digY, digW, digH, m2);
        
        // Pulsing Colon
        if (!_tmrRunning || (millis() / 500) % 2 == 0) {
            d.fillCircle(58, digY + 8, 2, SSD1306_WHITE);
            d.fillCircle(58, digY + 18, 2, SSD1306_WHITE);
        }
        
        _drawModernDigit(68, digY, digW, digH, s1);
        _drawModernDigit(85, digY, digW, digH, s2);
    }
    
    // --- 3. PROGRESS BAR & CONTROLS ---
    d.drawFastHLine(0, 45, SCREEN_W, SSD1306_WHITE);
    
    if (_tmrRunning && _tmrDuration > 0) {
        float pct = 1.0f - ((float)_tmrRemaining / _tmrDuration);
        if (pct < 0.0f) pct = 0.0f;
        else if (pct > 1.0f) pct = 1.0f;
        
        d.drawRoundRect(2, 48, 124, 6, 2, SSD1306_WHITE);
        int barW = (int)(pct * 120.0f);
        if (barW > 0) d.fillRect(4, 50, barW, 2, SSD1306_WHITE);
        
        d.setTextSize(1);
        d.setTextColor(SSD1306_WHITE);
        d.setCursor(4, 56);
        d.print("Tap:Stop  | Hold:Clock");
    } else if (_tmrRemaining == 0 && _tmrDuration > 0) {
        // TIME'S UP ALERT! (Pulsing flash)
        if ((millis() / 400) % 2 == 0) {
            d.fillRoundRect(4, 48, 120, 14, 2, SSD1306_WHITE);
            d.setTextColor(SSD1306_BLACK);
            d.setCursor(16, 51);
            d.print("TIME'S UP! DONE!");
        } else {
            d.drawRoundRect(4, 48, 120, 14, 2, SSD1306_WHITE);
            d.setTextColor(SSD1306_WHITE);
            d.setCursor(16, 51);
            d.print("TIME'S UP! DONE!");
        }
    } else {
        d.setTextSize(1);
        d.setTextColor(SSD1306_WHITE);
        d.setCursor(4, 49);
        d.print("Tap:Preset | Hold:Start");
        
        // Mini preset dots across the bottom (1m, 3m, 5m, 10m, 15m, 25m, 60m)
        int dotX = 20;
        for (int p = 0; p < 7; p++) {
            if (p == _tmrPresetIdx) {
                d.fillCircle(dotX + p * 14, 60, 2, SSD1306_WHITE);
            } else {
                d.drawPixel(dotX + p * 14, 60, SSD1306_WHITE);
            }
        }
    }
}
