// ============================================================================
// DeskBuddy — Multi-Page Message & AI Response Reader Implementation
// ============================================================================
#include "app_message.h"

AppMessage::AppMessage() {
    _currentPage = 0;
    _totalPages = 1;
    _shouldExit = false;
    _lastInteractionTime = 0;
}

void AppMessage::begin(DisplayManager* dm, FaceRenderer* face) {
    _dm = dm;
    _face = face;
}

void AppMessage::onEnter() {
    _currentPage = 0;
    _shouldExit = false;
    _lastInteractionTime = millis();
    
    // Set pet expression based on message emotion
    if (_face) {
        if (_emotion == "HAPPY" || _emotion == "LOVE") {
            _face->setExpression(Expression::HAPPY, 300);
        } else if (_emotion == "COOL") {
            _face->setExpression(Expression::COOL, 300);
        } else if (_emotion == "ANNOYED" || _emotion == "SAD") {
            _face->setExpression(Expression::SAD, 300);
        } else if (_emotion == "SLEEPY") {
            _face->setExpression(Expression::SLEEPY, 300);
        } else {
            _face->setExpression(Expression::HAPPY, 250);
        }
    }
}

void AppMessage::onExit() {
    _shouldExit = false;
}

void AppMessage::update() {
    // Auto-return to Pet Face after 60s of inactivity
    if (millis() - _lastInteractionTime > AUTO_EXIT_TIMEOUT_MS) {
        _shouldExit = true;
    }
}

void AppMessage::setMessage(const String& text, const String& emotion, const String& title) {
    _rawText = text;
    _emotion = emotion.length() > 0 ? emotion : "HAPPY";
    _title = title.length() > 0 ? title : "BUDDY AI";
    
    _lines.clear();
    _currentPage = 0;
    _shouldExit = false;
    _lastInteractionTime = millis();
    
    // Word-wrap text into lines of max 21 characters
    String curLine = "";
    int len = text.length();
    int wordStart = 0;
    
    for (int i = 0; i <= len; i++) {
        char c = (i < len) ? text[i] : ' ';
        
        if (c == ' ' || c == '\n' || c == '\r' || i == len) {
            if (i > wordStart) {
                String word = text.substring(wordStart, i);
                word.trim();
                
                if (word.length() > 0) {
                    // If a single word is longer than 21 chars, break it
                    while (word.length() > 21) {
                        if (curLine.length() > 0) {
                            _lines.push_back(curLine);
                            curLine = "";
                        }
                        _lines.push_back(word.substring(0, 21));
                        word = word.substring(21);
                    }
                    
                    if (curLine.length() == 0) {
                        curLine = word;
                    } else if (curLine.length() + 1 + word.length() <= 21) {
                        curLine += " " + word;
                    } else {
                        _lines.push_back(curLine);
                        curLine = word;
                    }
                }
            }
            
            if (c == '\n' && curLine.length() > 0) {
                _lines.push_back(curLine);
                curLine = "";
            }
            wordStart = i + 1;
        }
    }
    
    if (curLine.length() > 0) {
        _lines.push_back(curLine);
    }
    
    if (_lines.empty()) {
        _lines.push_back("No message text.");
    }
    
    _totalPages = (_lines.size() + LINES_PER_PAGE - 1) / LINES_PER_PAGE;
    if (_totalPages < 1) _totalPages = 1;
}

void AppMessage::handleTouch(TouchEvent event) {
    _lastInteractionTime = millis();
    
    if (event == TouchEvent::TAP) {
        if (_totalPages > 1) {
            // Cycle through pages smoothly!
            _currentPage = (_currentPage + 1) % _totalPages;
            if (_face) {
                _face->addHappiness(4);
            }
        }
    }
}

void AppMessage::render() {
    if (!_dm) return;
    Adafruit_SSD1306& d = _dm->display();
    
    d.setTextColor(SSD1306_WHITE);
    
    // ── 1. Header ──────────────────────────────────────────────
    // Title on left (bold look: print twice with 1px offset)
    d.setTextSize(1);
    d.setCursor(1, 1);
    d.print(_title);
    d.setCursor(0, 1);
    d.print(_title);  // double-draw = bold effect
    
    // Page counter on right
    if (_totalPages > 1) {
        String pageStr = String(_currentPage + 1) + "/" + String(_totalPages);
        int px = 128 - (pageStr.length() * 6) - 1;
        d.setCursor(px, 1);
        d.print(pageStr);
    } else {
        // Single page: show emotion tag
        String tag = "[" + _emotion.substring(0, 6) + "]";
        int px = 128 - (tag.length() * 6) - 1;
        d.setCursor(px, 1);
        d.print(tag);
    }
    
    // Solid divider line under header
    d.drawLine(0, 11, 127, 11, SSD1306_WHITE);
    d.drawLine(0, 12, 127, 12, SSD1306_WHITE); // double line = thicker
    
    // ── 2. Body Text ───────────────────────────────────────────
    // 4 lines, each 10px tall (font size 1 = 8px + 2px gap)
    int startLine = _currentPage * LINES_PER_PAGE;
    int y = 15;
    
    d.setTextSize(1);
    for (int i = 0; i < LINES_PER_PAGE; i++) {
        int lineIdx = startLine + i;
        if (lineIdx < (int)_lines.size()) {
            d.setCursor(1, y);
            d.print(_lines[lineIdx]);
        }
        y += 10;
    }
    
    // ── 3. Footer ──────────────────────────────────────────────
    if (_totalPages > 1) {
        d.drawLine(0, 54, 127, 54, SSD1306_WHITE);
        d.setTextSize(1);
        if (_currentPage + 1 < _totalPages) {
            // Center "TAP > Next" hint
            _dm->drawCenteredText("TAP > Next page", 56, 1);
        } else {
            _dm->drawCenteredText("TAP>Start | 2x>Exit", 56, 1);
        }
    }
}

