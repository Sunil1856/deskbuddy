// ============================================================================
// DeskBuddy — Coin Toss Game Implementation (Heads or Tails)
// ============================================================================
#include "game_toss.h"

GameToss::GameToss(DisplayManager* dm, Storage* storage, FaceRenderer* face)
    : _dm(dm), _storage(storage), _face(face) {
    for (int i = 0; i < MAX_SPARKLES; i++) {
        _sparkles[i].life = 0;
    }
}

void GameToss::onEnter() {
    _state = State::READY;
    _result = TossResult::NONE;
    _currentStreak = 0;
    _streakType = TossResult::NONE;
    if (_face) _face->setExpression(Expression::PLAYFUL, 200);
}

void GameToss::handleTouch(TouchEvent event) {
    if (event == TouchEvent::TOUCH_DOWN || event == TouchEvent::TAP) {
        if (_state == State::READY || _state == State::RESULT) {
            _startFlip();
        } else if (_state == State::FLIPPING) {
            // If user taps after mid-flip, fast-forward to result
            if (millis() - _flipStart > 250) {
                _finishFlip();
            }
        }
    }
}

void GameToss::_startFlip() {
    _state = State::FLIPPING;
    _flipStart = millis();
    _spinAngle = 0.0f;
    
    // Seed random with micros
    randomSeed(micros());
    
    // Initialize sparkles
    for (int i = 0; i < MAX_SPARKLES; i++) {
        _sparkles[i].x = 64.0f + (float)random(-8, 8);
        _sparkles[i].y = 34.0f + (float)random(-6, 6);
        _sparkles[i].vx = ((float)random(-14, 14)) / 10.0f;
        _sparkles[i].vy = ((float)random(-18, -6)) / 10.0f;
        _sparkles[i].life = random(12, 28);
    }
    
    if (_face) _face->setExpression(Expression::SURPRISED, 150);
}

void GameToss::_finishFlip() {
    _state = State::RESULT;
    
    // Deterministic random result from hardware RNG
    _result = (random(2) == 0) ? TossResult::HEADS : TossResult::TAILS;
    
    if (_result == TossResult::HEADS) {
        _headsCount++;
        if (_streakType == TossResult::HEADS) {
            _currentStreak++;
        } else {
            _streakType = TossResult::HEADS;
            _currentStreak = 1;
        }
        if (_face) {
            if (_currentStreak >= 3) {
                _face->setExpression(Expression::PARTY, 400);
            } else {
                _face->setExpression(Expression::EXCITED, 250);
            }
        }
    } else {
        _tailsCount++;
        if (_streakType == TossResult::TAILS) {
            _currentStreak++;
        } else {
            _streakType = TossResult::TAILS;
            _currentStreak = 1;
        }
        if (_face) {
            if (_currentStreak >= 3) {
                _face->setExpression(Expression::SMUG, 400);
            } else {
                _face->setExpression(Expression::WINK, 250);
            }
        }
    }
}

void GameToss::update() {
    if (_state != State::FLIPPING) return;
    
    unsigned long elapsed = millis() - _flipStart;
    if (elapsed >= FLIP_DURATION_MS) {
        _finishFlip();
        return;
    }
    
    // Decelerating spin physics
    float progress = (float)elapsed / FLIP_DURATION_MS;
    float spinIncrement = 0.65f * (1.0f - progress * 0.40f);
    _spinAngle += spinIncrement;
    
    // Update sparkles
    for (int i = 0; i < MAX_SPARKLES; i++) {
        if (_sparkles[i].life > 0) {
            _sparkles[i].x += _sparkles[i].vx;
            _sparkles[i].y += _sparkles[i].vy;
            _sparkles[i].vy += 0.2f; // subtle gravity on sparkles
            _sparkles[i].life--;
        }
    }
}

void GameToss::_drawCoin3D(int cx, int cy, float heightFactor, bool isHeads) {
    if (!_dm) return;
    Adafruit_SSD1306& d = _dm->display();
    
    const int rx = 15;
    int ry = max(2, (int)(abs(heightFactor) * 15.0f));
    
    if (ry <= 3) {
        // Edge-on view with 3D coin edge thickness
        d.drawRoundRect(cx - rx, cy - 2, rx * 2 + 1, 5, 2, SSD1306_WHITE);
        d.drawLine(cx - rx + 2, cy, cx + rx - 2, cy, SSD1306_WHITE);
    } else {
        // 3D perspective rounded ellipse
        d.drawRoundRect(cx - rx, cy - ry, rx * 2 + 1, ry * 2 + 1, ry, SSD1306_WHITE);
        if (ry > 6) {
            // Inner rim
            d.drawRoundRect(cx - rx + 2, cy - ry + 2, (rx - 2) * 2 + 1, (ry - 2) * 2 + 1, ry - 2, SSD1306_WHITE);
            // Face letter 'H' or 'T'
            d.setTextSize(1);
            d.setTextColor(SSD1306_WHITE);
            d.setCursor(cx - 3, cy - 3);
            d.print(isHeads ? "H" : "T");
        }
    }
}

void GameToss::_drawResultCoin(int cx, int cy, TossResult res) {
    if (!_dm) return;
    Adafruit_SSD1306& d = _dm->display();
    
    // Outer concentric coin rings
    d.drawCircle(cx, cy, 18, SSD1306_WHITE);
    d.drawCircle(cx, cy, 16, SSD1306_WHITE);
    
    // Mint marks at 12, 3, 6, 9 o'clock
    d.drawPixel(cx, cy - 17, SSD1306_WHITE);
    d.drawPixel(cx, cy + 17, SSD1306_WHITE);
    d.drawPixel(cx - 17, cy, SSD1306_WHITE);
    d.drawPixel(cx + 17, cy, SSD1306_WHITE);
    
    if (res == TossResult::HEADS) {
        // Crisp bold letter 'H'
        d.fillRect(cx - 7, cy - 8, 3, 16, SSD1306_WHITE); // Left post
        d.fillRect(cx + 4, cy - 8, 3, 16, SSD1306_WHITE); // Right post
        d.fillRect(cx - 4, cy - 2, 8, 4, SSD1306_WHITE);  // Crossbar
    } else {
        // Crisp bold letter 'T'
        d.fillRect(cx - 8, cy - 8, 16, 4, SSD1306_WHITE); // Top bar
        d.fillRect(cx - 2, cy - 4, 4, 12, SSD1306_WHITE); // Center stem
    }
}

void GameToss::render() {
    if (!_dm) return;
    Adafruit_SSD1306& d = _dm->display();
    
    if (_state == State::READY) {
        // Header
        d.setTextSize(1);
        d.setTextColor(SSD1306_WHITE);
        _dm->drawCenteredText("COIN TOSS", 2, 1);
        d.drawLine(0, 12, SCREEN_W, 12, SSD1306_WHITE);
        
        // Centered coin icon with question mark
        d.drawCircle(64, 32, 14, SSD1306_WHITE);
        d.drawCircle(64, 32, 12, SSD1306_WHITE);
        d.setCursor(61, 28);
        d.print("?");
        
        // Small decorative stars
        d.drawPixel(40, 26, SSD1306_WHITE);
        d.drawPixel(88, 26, SSD1306_WHITE);
        d.drawPixel(44, 38, SSD1306_WHITE);
        d.drawPixel(84, 38, SSD1306_WHITE);
        
        // Bottom prompt
        if ((millis() / 450) % 2 == 0) {
            _dm->drawCenteredText(">> TAP TO FLIP <<", 52, 1);
        } else {
            _dm->drawCenteredText("HOLD TO EXIT", 52, 1);
        }
    }
    else if (_state == State::FLIPPING) {
        // Header
        d.setTextSize(1);
        d.setTextColor(SSD1306_WHITE);
        _dm->drawCenteredText("FLIPPING...", 2, 1);
        d.drawLine(0, 12, SCREEN_W, 12, SSD1306_WHITE);
        
        // Parabolic flip trajectory
        float t = (float)(millis() - _flipStart) / FLIP_DURATION_MS;
        if (t > 1.0f) t = 1.0f;
        float arc = sin(t * 3.14159f);
        int cy = 36 - (int)(arc * 20.0f);
        
        // Floor shadow shrinks as coin rises
        int shadowW = 14 - (int)(arc * 8.0f);
        d.drawFastHLine(64 - shadowW, 57, shadowW * 2 + 1, SSD1306_WHITE);
        
        // Render sparkle particles
        for (int i = 0; i < MAX_SPARKLES; i++) {
            if (_sparkles[i].life > 0) {
                int sx = (int)_sparkles[i].x;
                int sy = (int)_sparkles[i].y;
                if (sx >= 0 && sx < SCREEN_W && sy >= 14 && sy < SCREEN_H) {
                    d.drawPixel(sx, sy, SSD1306_WHITE);
                }
            }
        }
        
        // 3D spinning coin
        float hf = cos(_spinAngle);
        _drawCoin3D(64, cy, hf, hf >= 0);
    }
    else if (_state == State::RESULT) {
        // Result coin medal on the left
        _drawResultCoin(30, 32, _result);
        
        // Inverted banner badge on the right
        d.fillRoundRect(56, 4, 70, 16, 3, SSD1306_WHITE);
        d.setTextColor(SSD1306_BLACK);
        d.setTextSize(1);
        d.setCursor(64, 8);
        if (_result == TossResult::HEADS) {
            d.print(">> HEADS <<");
        } else {
            d.print(">> TAILS <<");
        }
        
        // Stats tally below banner
        d.setTextColor(SSD1306_WHITE);
        d.setCursor(58, 25);
        d.printf("H: %-3d  T: %-3d", _headsCount, _tailsCount);
        
        // Streak counter or total
        d.setCursor(58, 38);
        if (_currentStreak >= 2) {
            d.printf("%dx %s STREAK!", _currentStreak, _streakType == TossResult::HEADS ? "H" : "T");
        } else {
            d.printf("Total: %d", _headsCount + _tailsCount);
        }
        
        // Bottom prompt
        d.setCursor(58, 52);
        d.print("TAP: FLIP AGAIN");
    }
}
