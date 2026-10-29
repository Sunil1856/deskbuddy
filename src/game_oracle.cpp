// ============================================================================
// DeskBuddy — Magic Oracle Game Implementation (Cyber 8-Ball & Decision Maker)
// ============================================================================
#include "game_oracle.h"

struct OracleAnswer {
    const char* text;
    Expression expr;
};

static const OracleAnswer ORACLE_ANSWERS[] = {
    { "100% KAR DE!",   Expression::PARTY },
    { "BIG YES!",       Expression::EXCITED },
    { "DEFINITELY",     Expression::COOL },
    { "SIGNS SAY YES",  Expression::HAPPY },
    { "PAKKA HOGA!",    Expression::LOVE },
    { "FULL POWER!",    Expression::DETERMINED },
    { "CHANCE 99%",     Expression::STARRY },
    { "TRUST YOURSELF", Expression::CHILL },
    { "BHAI SOCH LE!",  Expression::SKEPTICAL },
    { "NO WAY BRO!",    Expression::SHOCKED },
    { "DOUBTFUL...",    Expression::CONFUSED },
    { "HARD PASS",      Expression::ANGRY },
    { "ABHI NAHI",      Expression::SLEEPY },
    { "NEVER EVER",     Expression::DEAD },
    { "BETTER NOT",     Expression::SWEAT },
    { "CHAI PEE LE!",   Expression::PLAYFUL },
    { "ASK AGAIN",      Expression::NERD },
    { "SECRET! SHH",    Expression::SMUG }
};
static const int ORACLE_COUNT = sizeof(ORACLE_ANSWERS) / sizeof(ORACLE_ANSWERS[0]);

GameOracle::GameOracle(DisplayManager* dm, Storage* storage, FaceRenderer* face)
    : _dm(dm), _storage(storage), _face(face) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        _particles[i].life = 0;
    }
}

void GameOracle::onEnter() {
    _state = State::READY;
    _answerIndex = 0;
    if (_face) _face->setExpression(Expression::STARRY, 200);
}

void GameOracle::handleTouch(TouchEvent event) {
    if (event == TouchEvent::TOUCH_DOWN || event == TouchEvent::TAP) {
        if (_state == State::READY || _state == State::REVEAL) {
            _startConsultation();
        } else if (_state == State::THINKING) {
            // Quick skip to reveal if user taps mid-thought
            if (millis() - _thinkStart > 300) {
                _revealFortune();
            }
        }
    }
}

void GameOracle::_startConsultation() {
    _state = State::THINKING;
    _thinkStart = millis();
    _vortexAngle = 0.0f;
    randomSeed(micros());
    
    // Spawn swirling cosmic particles around the 8-ball
    for (int i = 0; i < MAX_PARTICLES; i++) {
        _particles[i].angle = ((float)i / MAX_PARTICLES) * 6.28318f;
        _particles[i].dist = 22.0f + (float)random(6, 18);
        _particles[i].speed = 0.12f + ((float)random(2, 8)) / 100.0f;
        _particles[i].life = random(20, 40);
    }
    
    if (_face) _face->setExpression(Expression::CONFUSED, 200);
}

void GameOracle::_revealFortune() {
    _state = State::REVEAL;
    
    // Randomly pick answer
    _answerIndex = random(ORACLE_COUNT);
    
    // Trigger Buddy's reactive emotion
    if (_face) {
        _face->setExpression(ORACLE_ANSWERS[_answerIndex].expr, 350);
    }
}

void GameOracle::update() {
    if (_state != State::THINKING) return;
    
    unsigned long elapsed = millis() - _thinkStart;
    if (elapsed >= THINK_DURATION_MS) {
        _revealFortune();
        return;
    }
    
    // Physical spring shake
    float progress = (float)elapsed / THINK_DURATION_MS;
    float dampening = 1.0f - progress * 0.35f;
    _shakeX = sin(elapsed * 0.038f) * 3.2f * dampening;
    _shakeY = cos(elapsed * 0.046f) * 1.8f * dampening;
    
    // Update cosmic vortex particles (spiral inward)
    for (int i = 0; i < MAX_PARTICLES; i++) {
        _particles[i].angle += _particles[i].speed;
        _particles[i].dist -= 0.22f; // spiral towards center
        if (_particles[i].dist < 12.0f) {
            _particles[i].dist = 28.0f; // respawn at outer rim
        }
    }
}

void GameOracle::_draw8Ball(int cx, int cy, int radius) {
    if (!_dm) return;
    Adafruit_SSD1306& d = _dm->display();
    
    // Outer shaded sphere
    d.drawCircle(cx, cy, radius, SSD1306_WHITE);
    d.drawCircle(cx, cy, radius - 2, SSD1306_WHITE);
    
    // Specular highlight gleam at top-left
    d.drawPixel(cx - radius / 2, cy - radius / 2, SSD1306_WHITE);
    d.drawPixel(cx - radius / 2 + 1, cy - radius / 2, SSD1306_WHITE);
    d.drawPixel(cx - radius / 2, cy - radius / 2 + 1, SSD1306_WHITE);
    
    // Center white viewport circle
    int innerR = radius / 2;
    d.fillCircle(cx, cy, innerR, SSD1306_WHITE);
    
    // Classic black '8' inside the white circle
    d.setTextColor(SSD1306_BLACK);
    d.setTextSize(1);
    d.setCursor(cx - 2, cy - 3);
    d.print("8");
}

void GameOracle::render() {
    if (!_dm) return;
    Adafruit_SSD1306& d = _dm->display();
    
    if (_state == State::READY) {
        // Header
        d.setTextSize(1);
        d.setTextColor(SSD1306_WHITE);
        _dm->drawCenteredText("MAGIC ORACLE", 2, 1);
        d.drawLine(0, 12, SCREEN_W, 12, SSD1306_WHITE);
        
        // Centered 8-ball
        _draw8Ball(64, 33, 16);
        
        // Mystical celestial accent dots
        d.drawPixel(36, 26, SSD1306_WHITE);
        d.drawPixel(92, 26, SSD1306_WHITE);
        d.drawPixel(42, 40, SSD1306_WHITE);
        d.drawPixel(86, 40, SSD1306_WHITE);
        
        // Animated prompt
        if ((millis() / 450) % 2 == 0) {
            _dm->drawCenteredText(">> ASK & TAP <<", 53, 1);
        } else {
            _dm->drawCenteredText("HOLD TO EXIT", 53, 1);
        }
    }
    else if (_state == State::THINKING) {
        // Header
        d.setTextSize(1);
        d.setTextColor(SSD1306_WHITE);
        _dm->drawCenteredText("CONSULTING...", 2, 1);
        d.drawLine(0, 12, SCREEN_W, 12, SSD1306_WHITE);
        
        int bx = 64 + (int)_shakeX;
        int by = 34 + (int)_shakeY;
        
        // Render cosmic vortex particles spiraling into the ball
        for (int i = 0; i < MAX_PARTICLES; i++) {
            int px = 64 + (int)(cos(_particles[i].angle) * _particles[i].dist);
            int py = 34 + (int)(sin(_particles[i].angle) * _particles[i].dist);
            if (px >= 0 && px < SCREEN_W && py >= 14 && py < SCREEN_H) {
                d.drawPixel(px, py, SSD1306_WHITE);
            }
        }
        
        // Shaking 8-Ball
        _draw8Ball(bx, by, 15);
    }
    else if (_state == State::REVEAL) {
        // Frame card
        d.drawRoundRect(6, 12, 116, 36, 4, SSD1306_WHITE);
        d.drawRoundRect(8, 14, 112, 32, 2, SSD1306_WHITE);
        
        // Title header badge
        d.fillRoundRect(32, 5, 64, 13, 2, SSD1306_WHITE);
        d.setTextColor(SSD1306_BLACK);
        d.setTextSize(1);
        d.setCursor(36, 8);
        d.print("ORACLE SAYS");
        
        // Centered Prediction Result
        d.setTextColor(SSD1306_WHITE);
        d.setTextSize(1);
        _dm->drawCenteredText(ORACLE_ANSWERS[_answerIndex].text, 26, 1);
        
        // Subtext decorative stars
        d.drawPixel(14, 30, SSD1306_WHITE);
        d.drawPixel(113, 30, SSD1306_WHITE);
        
        // Bottom prompt
        _dm->drawCenteredText(">> TAP: ASK AGAIN <<", 53, 1);
    }
}
