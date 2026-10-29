// ============================================================================
// DeskBuddy — Face Renderer Implementation (30 Procedural Expressions)
// ============================================================================
#include "face_renderer.h"
#include "storage.h"
#include "time_service.h"
#include "easing.h"
#include "icons.h"
#include <math.h>

// Default eye positions (centered on 128x64 screen)
#define LEFT_EYE_X    40.0f
#define RIGHT_EYE_X   88.0f
#define EYE_Y         27.0f
#define MOUTH_X       64.0f
#define MOUTH_Y       53.0f

void FaceRenderer::begin(DisplayManager* dm, Storage* storage) {
    _dm = dm;
    _storage = storage;
    
    if (_storage) {
        _happiness = _storage->getHappiness();
    } else {
        _happiness = 75;
    }
    
    _lastActivityTime = millis();
    _nextBlinkTime = millis() + random(BLINK_MIN_INTERVAL_MS, BLINK_MAX_INTERVAL_MS);
    _nextLookTime = millis() + random(1000, IDLE_LOOK_INTERVAL_MS);
    _nextSpontaneousExprTime = millis() + random(15000, 30000);
    
    // Initialize base neutral params
    _setExpressionParams(Expression::NEUTRAL, _leftEye, _rightEye, _mouth, _extras);
    _targetLeftEye = _leftEye;
    _targetRightEye = _rightEye;
    _targetMouth = _mouth;
    _targetExtras = _extras;
    _transProgress = 1.0f;
    
    updateMood();
}

void FaceRenderer::setHappiness(uint8_t val) {
    if (val > 100) val = 100;
    _happiness = val;
    if (_storage) _storage->setHappiness(_happiness);
    updateMood();
}

void FaceRenderer::addHappiness(int delta) {
    int newH = (int)_happiness + delta;
    if (newH > 100) newH = 100;
    if (newH < 0) newH = 0;
    _happiness = (uint8_t)newH;
    if (_storage) _storage->setHappiness(_happiness);
    updateMood();
}

void FaceRenderer::updateMood() {
    if (_manualHold) return; // Do not overwrite user-selected expression!
    
    if (_happiness >= 80) {
        if (_targetExpr != Expression::LOVE && _targetExpr != Expression::PARTY && 
            _targetExpr != Expression::EXCITED && _targetExpr != Expression::HEARTBEAT && 
            _targetExpr != Expression::LAUGHING && _targetExpr != Expression::SINGING) {
            setExpression(Expression::HAPPY, 300);
        }
    } else if (_happiness >= 40) {
        if (_targetExpr != Expression::CHILL && _targetExpr != Expression::PLAYFUL && 
            _targetExpr != Expression::SMUG && _targetExpr != Expression::NERD) {
            setExpression(Expression::NEUTRAL, 300);
        }
    } else {
        if (_targetExpr != Expression::ANGRY && _targetExpr != Expression::PLEADING &&
            _targetExpr != Expression::DEAD) {
            setExpression(Expression::SAD, 300);
        }
    }
}

const char* FaceRenderer::getExpressionName(Expression expr) const {
    switch (expr) {
        case Expression::NEUTRAL:    return "Neutral";
        case Expression::HAPPY:      return "Happy";
        case Expression::LOVE:       return "Love";
        case Expression::SLEEPY:     return "Sleepy";
        case Expression::SLEEPING:   return "Sleeping";
        case Expression::SURPRISED:  return "Surprised";
        case Expression::ANGRY:      return "Angry";
        case Expression::SAD:        return "Sad";
        case Expression::CONFUSED:   return "Confused";
        case Expression::EXCITED:    return "Excited";
        case Expression::WINK:       return "Wink";
        case Expression::DIZZY:      return "Dizzy";
        case Expression::COOL:       return "Cool";
        case Expression::BLUSHING:   return "Blushing";
        case Expression::SKEPTICAL:  return "Skeptical";
        case Expression::STARRY:     return "Starry";
        case Expression::PLAYFUL:    return "Playful";
        case Expression::CHILL:      return "Chill";
        case Expression::PARTY:      return "Party";
        case Expression::SHOCKED:    return "Shocked!";
        case Expression::SMUG:       return "Smug :3";
        case Expression::PLEADING:   return "Pleading";
        case Expression::NERD:       return "Smart Nerd";
        case Expression::SWEAT:      return "Sweating";
        case Expression::MONEY:      return "Rich $$";
        case Expression::DEAD:       return "KO Dead X_X";
        case Expression::DETERMINED: return "Determined";
        case Expression::SINGING:    return "Singing ~";
        case Expression::HEARTBEAT:  return "Heartbeat";
        case Expression::LAUGHING:   return "Laughing XD";
        default: return "Buddy";
    }
}

void FaceRenderer::setExpression(Expression expr, uint16_t transitionMs) {
    if (expr == _targetExpr && _transProgress >= 1.0f) return;
    
    _targetExpr = expr;
    _transDuration = transitionMs;
    _transStartTime = millis();
    _transProgress = 0.0f;
    
    _setExpressionParams(expr, _targetLeftEye, _targetRightEye, _targetMouth, _targetExtras);
}

void FaceRenderer::cycleNextExpression() {
    int next = ((int)_targetExpr + 1) % (int)Expression::COUNT;
    _manualHold = true;
    _isIdleSleeping = false;
    _isIdleSleepy = false;
    _lastActivityTime = millis();
    setExpression((Expression)next, 200);
    _toastStartTime = millis();
    if (_happiness < 100) _happiness += 2;
}

void FaceRenderer::_setExpressionParams(Expression expr, EyeParams& le, EyeParams& re, MouthParams& m, FaceExtras& ex) {
    // Reset all defaults
    le = {LEFT_EYE_X, EYE_Y, 18, 16, 0, 0, 5, 1.0f, 0, 0.3f, false, false, false, false, false, false, false, false, false};
    re = {RIGHT_EYE_X, EYE_Y, 18, 16, 0, 0, 5, 1.0f, 0, 0.3f, false, false, false, false, false, false, false, false, false};
    m = {MOUTH_X, MOUTH_Y, 20, 0.2f, 0, false, false, false, false};
    ex = {false, false, false, false, false, false, 0.0f};
    
    switch (expr) {
        case Expression::NEUTRAL:
            break;
            
        case Expression::HAPPY:
            le.height = 12; le.squint = 0.4f; le.openness = 0.7f;
            re.height = 12; re.squint = 0.4f; re.openness = 0.7f;
            m.curve = 0.8f; m.width = 24;
            break;
            
        case Expression::LOVE:
            le.isHeart = true; le.width = 20; le.height = 18;
            re.isHeart = true; re.width = 20; re.height = 18;
            m.curve = 1.0f; m.width = 24;
            break;
            
        case Expression::SLEEPY:
            le.openness = 0.35f; le.y = EYE_Y + 3;
            re.openness = 0.35f; re.y = EYE_Y + 3;
            m.curve = 0.0f; m.openness = 0.6f; m.width = 12; // cute drowsy yawn
            ex.showBlush = true;
            ex.blushIntensity = 0.4f;
            break;
            
        case Expression::SLEEPING:
            le.openness = 0.0f;
            re.openness = 0.0f;
            m.curve = 0.35f; m.width = 12; m.openness = 0.0f; // gentle sweet sleep smile
            ex.showZzz = true;
            ex.showBlush = true;
            ex.blushIntensity = 0.5f;
            break;
            
        case Expression::SURPRISED:
            le.width = 22; le.height = 22; le.pupilSize = 3;
            re.width = 22; re.height = 22; re.pupilSize = 3;
            m.curve = 0.0f; m.openness = 0.9f; m.width = 14;
            break;
            
        case Expression::ANGRY:
            le.squint = 0.5f; le.y = EYE_Y - 2; le.pupilSize = 4;
            re.squint = 0.5f; re.y = EYE_Y - 2; re.pupilSize = 4;
            m.curve = -0.6f; m.isZigzag = true; m.width = 22;
            break;
            
        case Expression::SAD:
            le.y = EYE_Y + 2; le.height = 14;
            re.y = EYE_Y + 2; re.height = 14;
            m.curve = -0.8f; m.width = 20;
            ex.showTear = true;
            break;
            
        case Expression::CONFUSED:
            le.width = 16; le.height = 16;
            re.width = 20; re.height = 12;
            le.pupilX = -0.3f;
            re.pupilX = 0.3f;
            m.isWavy = true; m.width = 22;
            break;
            
        case Expression::EXCITED:
            le.width = 20; le.height = 20; le.pupilSize = 6;
            re.width = 20; re.height = 20; re.pupilSize = 6;
            le.y = EYE_Y - 2; re.y = EYE_Y - 2;
            m.curve = 1.0f; m.openness = 0.5f; m.width = 28;
            ex.showExclaim = true;
            break;
            
        case Expression::WINK:
            le.openness = 1.0f;
            re.openness = 0.0f;
            m.curve = 0.6f; m.width = 22;
            break;
            
        case Expression::DIZZY:
            le.isSpiral = true; le.width = 18; le.height = 18;
            re.isSpiral = true; re.width = 18; re.height = 18;
            m.isWavy = true; m.width = 18;
            break;
            
        case Expression::COOL:
            le.isCool = true; le.width = 22; le.height = 10; le.y = EYE_Y - 1;
            re.isCool = true; re.width = 22; re.height = 10; re.y = EYE_Y - 1;
            m.curve = 0.4f; m.width = 16;
            break;
            
        case Expression::BLUSHING:
            le.openness = 0.8f; le.squint = 0.2f;
            re.openness = 0.8f; re.squint = 0.2f;
            m.curve = 0.5f; m.width = 16;
            ex.showBlush = true; ex.blushIntensity = 1.0f;
            break;
            
        case Expression::SKEPTICAL:
            le.y = EYE_Y - 3; le.height = 12; le.squint = 0.3f;
            re.y = EYE_Y + 1; re.height = 14; re.squint = 0.6f;
            m.curve = 0.1f; m.width = 14;
            m.x = MOUTH_X + 5;
            break;
            
        case Expression::STARRY:
            le.isStar = true; le.width = 20; le.height = 20;
            re.isStar = true; re.width = 20; re.height = 20;
            m.curve = 1.0f; m.openness = 0.3f; m.width = 28;
            break;
            
        case Expression::PLAYFUL:
            le.openness = 0.7f; le.squint = 0.3f;
            re.openness = 0.0f;
            m.curve = 0.7f; m.width = 22; m.hasTongue = true;
            break;
            
        case Expression::CHILL:
            le.openness = 0.5f; le.squint = 0.2f; le.height = 12;
            re.openness = 0.5f; re.squint = 0.2f; re.height = 12;
            m.curve = 0.3f; m.width = 16;
            break;
            
        case Expression::PARTY:
            le.isStar = true; le.width = 22; le.height = 22;
            re.isStar = true; re.width = 22; re.height = 22;
            m.curve = 1.0f; m.openness = 0.8f; m.width = 28; m.hasTongue = true;
            ex.showExclaim = true;
            break;
            
        case Expression::SHOCKED:
            le.width = 22; le.height = 24; le.pupilSize = 2;
            re.width = 22; re.height = 24; re.pupilSize = 2;
            m.curve = -0.2f; m.openness = 0.95f; m.width = 16;
            break;
            
        case Expression::SMUG:
            le.y = EYE_Y - 2; le.squint = 0.4f; le.height = 11;
            re.y = EYE_Y - 1; re.squint = 0.2f; re.height = 12;
            m.isCatSmirk = true; m.width = 22;
            break;
            
        case Expression::PLEADING:
            le.isPleading = true; le.width = 24; le.height = 22;
            re.isPleading = true; re.width = 24; re.height = 22;
            m.curve = -0.4f; m.width = 14;
            ex.showTear = true;
            break;
            
        case Expression::NERD:
            le.isNerd = true; le.width = 20; le.height = 18;
            re.isNerd = true; re.width = 20; re.height = 18;
            m.curve = 0.7f; m.width = 18;
            break;
            
        case Expression::SWEAT:
            le.squint = 0.3f; le.openness = 0.8f;
            re.squint = 0.3f; re.openness = 0.8f;
            m.curve = 0.3f; m.width = 16;
            ex.showSweat = true;
            break;
            
        case Expression::MONEY:
            le.isDollar = true; le.width = 20; le.height = 20;
            re.isDollar = true; re.width = 20; re.height = 20;
            m.curve = 1.0f; m.openness = 0.6f; m.width = 26;
            break;
            
        case Expression::DEAD:
            le.isCross = true; le.width = 18; le.height = 18;
            re.isCross = true; re.width = 18; re.height = 18;
            m.curve = 0.0f; m.width = 20; m.hasTongue = true;
            break;
            
        case Expression::DETERMINED:
            le.y = EYE_Y - 1; le.squint = 0.4f; le.pupilSize = 4;
            re.y = EYE_Y - 1; re.squint = 0.4f; re.pupilSize = 4;
            m.curve = 0.0f; m.width = 20; m.isZigzag = true;
            break;
            
        case Expression::SINGING:
            le.openness = 0.0f;
            re.openness = 0.0f;
            m.curve = 0.0f; m.openness = 0.85f; m.width = 12; // circle singing mouth
            ex.showMusic = true;
            break;
            
        case Expression::HEARTBEAT:
            le.isHeart = true; le.width = 22; le.height = 20;
            re.isHeart = true; re.width = 22; re.height = 20;
            m.curve = 0.9f; m.width = 24;
            ex.showBlush = true; ex.blushIntensity = 1.0f;
            break;
            
        case Expression::LAUGHING:
            le.isSquintV = true; le.width = 18; le.height = 14;
            re.isSquintV = true; re.width = 18; re.height = 14;
            m.curve = 1.0f; m.openness = 0.8f; m.width = 26; m.hasTongue = true;
            break;
            
        default:
            break;
    }
}

void FaceRenderer::update() {
    unsigned long now = millis();
    
    // Update expression transition
    if (_transProgress < 1.0f) {
        float elapsed = (float)(now - _transStartTime);
        _transProgress = Easing::clamp(elapsed / _transDuration, 0.0f, 1.0f);
        _interpolateParams(Easing::easeInOutCubic(_transProgress));
        if (_transProgress >= 1.0f) {
            _currentExpr = _targetExpr;
        }
    }
    
    // Breathing & floating oscillations
    _breathPhase += 0.05f;
    if (_breathPhase > 2 * M_PI) _breathPhase -= 2 * M_PI;
    
    _updateBlink();
    _updateIdleLook();
    _updateIdleTimeout();
    
    if (_extras.showZzz) {
        _updateZzz();
    }
    
    if (_extras.showMusic) {
        _musicPhase += 0.08f;
        if (_musicPhase > 2 * M_PI) _musicPhase -= 2 * M_PI;
    }
    
    if (_leftEye.isSpiral || _rightEye.isSpiral) {
        _spiralAngle += 0.15f;
        if (_spiralAngle > 2 * M_PI) _spiralAngle -= 2 * M_PI;
    }
    
    // Handle touch reaction timeout
    if (_touchReacting && (now - _touchReactTime > 2000)) {
        _touchReacting = false;
        if (_currentExpr == Expression::LOVE || _currentExpr == Expression::BLUSHING ||
            _currentExpr == Expression::SURPRISED) {
            setExpression(Expression::HAPPY, 500);
        }
    }
    
    // Spontaneous autonomous pet life animation (plays all 30 expressions naturally!)
    if (!_isIdleSleeping && !_isIdleSleepy && !_manualHold && !_touchReacting) {
        if (!_isSpontaneousActive && now >= _nextSpontaneousExprTime) {
            // Pick a random fun expression from the 30 expressions (excluding SLEEPING/DEAD)
            static const Expression FUN_EXPRS[] = {
                Expression::HAPPY, Expression::LOVE, Expression::SURPRISED, Expression::CONFUSED,
                Expression::EXCITED, Expression::WINK, Expression::COOL, Expression::BLUSHING,
                Expression::SKEPTICAL, Expression::STARRY, Expression::PLAYFUL, Expression::CHILL,
                Expression::PARTY, Expression::SMUG, Expression::PLEADING, Expression::NERD,
                Expression::SINGING, Expression::HEARTBEAT, Expression::LAUGHING
            };
            int rIdx = random(sizeof(FUN_EXPRS) / sizeof(FUN_EXPRS[0]));
            setExpression(FUN_EXPRS[rIdx], 300);
            _isSpontaneousActive = true;
            _spontaneousEndTime = now + random(3500, 5500); // hold for 3.5 - 5.5s
        } else if (_isSpontaneousActive && now >= _spontaneousEndTime) {
            _isSpontaneousActive = false;
            _nextSpontaneousExprTime = now + random(20000, 40000);
            updateMood(); // smoothly return to base Happy/Neutral
        }
    }
}

void FaceRenderer::_interpolateParams(float t) {
    auto lerpF = [](float a, float b, float t) { return a + (b - a) * t; };
    
    // Left eye
    _leftEye.x = lerpF(_leftEye.x, _targetLeftEye.x, t);
    _leftEye.y = lerpF(_leftEye.y, _targetLeftEye.y, t);
    _leftEye.width = lerpF(_leftEye.width, _targetLeftEye.width, t);
    _leftEye.height = lerpF(_leftEye.height, _targetLeftEye.height, t);
    _leftEye.pupilSize = lerpF(_leftEye.pupilSize, _targetLeftEye.pupilSize, t);
    _leftEye.openness = lerpF(_leftEye.openness, _targetLeftEye.openness, t);
    _leftEye.squint = lerpF(_leftEye.squint, _targetLeftEye.squint, t);
    
    // Right eye
    _rightEye.x = lerpF(_rightEye.x, _targetRightEye.x, t);
    _rightEye.y = lerpF(_rightEye.y, _targetRightEye.y, t);
    _rightEye.width = lerpF(_rightEye.width, _targetRightEye.width, t);
    _rightEye.height = lerpF(_rightEye.height, _targetRightEye.height, t);
    _rightEye.pupilSize = lerpF(_rightEye.pupilSize, _targetRightEye.pupilSize, t);
    _rightEye.openness = lerpF(_rightEye.openness, _targetRightEye.openness, t);
    _rightEye.squint = lerpF(_rightEye.squint, _targetRightEye.squint, t);
    
    if (t > 0.5f) {
        _leftEye.isHeart = _targetLeftEye.isHeart;
        _leftEye.isStar = _targetLeftEye.isStar;
        _leftEye.isSpiral = _targetLeftEye.isSpiral;
        _leftEye.isCool = _targetLeftEye.isCool;
        _leftEye.isCross = _targetLeftEye.isCross;
        _leftEye.isDollar = _targetLeftEye.isDollar;
        _leftEye.isPleading = _targetLeftEye.isPleading;
        _leftEye.isNerd = _targetLeftEye.isNerd;
        _leftEye.isSquintV = _targetLeftEye.isSquintV;
        
        _rightEye.isHeart = _targetRightEye.isHeart;
        _rightEye.isStar = _targetRightEye.isStar;
        _rightEye.isSpiral = _targetRightEye.isSpiral;
        _rightEye.isCool = _targetRightEye.isCool;
        _rightEye.isCross = _targetRightEye.isCross;
        _rightEye.isDollar = _targetRightEye.isDollar;
        _rightEye.isPleading = _targetRightEye.isPleading;
        _rightEye.isNerd = _targetRightEye.isNerd;
        _rightEye.isSquintV = _targetRightEye.isSquintV;
        
        _mouth.isWavy = _targetMouth.isWavy;
        _mouth.isZigzag = _targetMouth.isZigzag;
        _mouth.hasTongue = _targetMouth.hasTongue;
        _mouth.isCatSmirk = _targetMouth.isCatSmirk;
        _extras = _targetExtras;
    }
    
    // Mouth
    _mouth.x = lerpF(_mouth.x, _targetMouth.x, t);
    _mouth.y = lerpF(_mouth.y, _targetMouth.y, t);
    _mouth.width = lerpF(_mouth.width, _targetMouth.width, t);
    _mouth.curve = lerpF(_mouth.curve, _targetMouth.curve, t);
    _mouth.openness = lerpF(_mouth.openness, _targetMouth.openness, t);
}

void FaceRenderer::_updateBlink() {
    unsigned long now = millis();
    if (_currentExpr == Expression::SLEEPING || _currentExpr == Expression::DEAD) return;
    
    if (_blinkProgress >= 1.0f && !_blinkClosing) {
        if (now >= _nextBlinkTime) {
            _blinkClosing = true;
            _blinkStartTime = now;
            _doDoubleBlink = (random(100) < 15);
            _doubleBlinkCount = 0;
        }
    }
    
    if (_blinkClosing) {
        float elapsed = (float)(now - _blinkStartTime);
        if (elapsed < BLINK_CLOSE_MS) {
            _blinkProgress = 1.0f - (elapsed / BLINK_CLOSE_MS);
        } else if (elapsed < BLINK_CLOSE_MS + BLINK_OPEN_MS) {
            float openElapsed = elapsed - BLINK_CLOSE_MS;
            _blinkProgress = openElapsed / BLINK_OPEN_MS;
        } else {
            _blinkProgress = 1.0f;
            _blinkClosing = false;
            _doubleBlinkCount++;
            
            if (_doDoubleBlink && _doubleBlinkCount < 2) {
                _blinkClosing = true;
                _blinkStartTime = now + 50;
            } else {
                _nextBlinkTime = now + random(BLINK_MIN_INTERVAL_MS, BLINK_MAX_INTERVAL_MS);
            }
        }
    }
}

void FaceRenderer::_updateIdleLook() {
    unsigned long now = millis();
    if (_currentExpr == Expression::SLEEPING || _currentExpr == Expression::DIZZY || _currentExpr == Expression::DEAD) return;
    
    if (now >= _nextLookTime) {
        _targetLookX = (float)random(-100, 101) / 100.0f * 0.5f;
        _targetLookY = (float)random(-50, 51) / 100.0f * 0.3f;
        _lookTransStart = now;
        _nextLookTime = now + random(2000, 5000);
    }
    
    float lookElapsed = (float)(now - _lookTransStart) / 500.0f;
    lookElapsed = Easing::clamp(lookElapsed, 0.0f, 1.0f);
    float eased = Easing::easeInOutSine(lookElapsed);
    _lookX = Easing::lerp(_lookX, _targetLookX, eased * 0.1f);
    _lookY = Easing::lerp(_lookY, _targetLookY, eased * 0.1f);
}

void FaceRenderer::_updateIdleTimeout() {
    unsigned long now = millis();
    unsigned long idle = now - _lastActivityTime;
    
    uint32_t sleepTimeout = _storage ? _storage->getSleepTimeout() : SLEEP_TIMEOUT_MS;
    uint32_t sleepyTimeout = sleepTimeout > 60000 ? (sleepTimeout - 30000) : (sleepTimeout / 2);
    
    if (idle > sleepTimeout && !_isIdleSleeping) {
        goToSleep();
    } else if (idle > sleepyTimeout && !_isIdleSleepy) {
        _isIdleSleepy = true;
        setExpression(Expression::SLEEPY, 500);
    }
}

void FaceRenderer::_updateZzz() {
    _zzzPhase += 0.05f;
    if (_zzzPhase > 2 * M_PI) _zzzPhase -= 2 * M_PI;
}

void FaceRenderer::resetIdleTimer() {
    _lastActivityTime = millis();
    if (_isIdleSleeping || _isIdleSleepy) {
        _isIdleSleeping = false;
        _isIdleSleepy = false;
        setExpression(Expression::SURPRISED, 200);
    }
}

void FaceRenderer::goToSleep() {
    _isIdleSleeping = true;
    _isIdleSleepy = true;
    _manualHold = false;
    _extras.showZzz = true;
    setExpression(Expression::SLEEPING, 1000);
}

void FaceRenderer::wakeUp() {
    _isIdleSleeping = false;
    _isIdleSleepy = false;
    _extras.showZzz = false;
    _lastActivityTime = millis();
    setExpression(Expression::HAPPY, 300);
}

void FaceRenderer::onTouch() {
    resetIdleTimer();
    addHappiness(5);
    _touchCount++;
    unsigned long now = millis();
    
    if (now - _lastTouchTime < 3000) {
        if (_touchCount >= 3) {
            setExpression(Expression::LOVE, 200);
            _touchReacting = true;
            _touchReactTime = now;
            _touchCount = 0;
        }
    } else {
        _touchCount = 1;
    }
    _lastTouchTime = now;
    
    if (!_touchReacting) {
        setExpression(Expression::HAPPY, 150);
        _touchReacting = true;
        _touchReactTime = now;
    }
}

void FaceRenderer::onLongTouch() {
    resetIdleTimer();
    addHappiness(15);
    setExpression(Expression::BLUSHING, 300);
    _touchReacting = true;
    _touchReactTime = millis();
}

void FaceRenderer::onWifiConnect() {
    setExpression(Expression::EXCITED, 200);
    _touchReacting = true;
    _touchReactTime = millis();
}

void FaceRenderer::onWifiFail() {
    setExpression(Expression::CONFUSED, 300);
    _touchReacting = true;
    _touchReactTime = millis();
}

void FaceRenderer::onGameOver() {
    setExpression(Expression::DIZZY, 200);
    _touchReacting = true;
    _touchReactTime = millis();
}

void FaceRenderer::onGoodWeather() {
    setExpression(Expression::COOL, 300);
    _touchReacting = true;
    _touchReactTime = millis();
}

// ======================== RENDERING ========================

void FaceRenderer::render() {
    if (!_dm) return;
    Adafruit_SSD1306& d = _dm->display();
    
    // Apply blink to eye openness
    EyeParams leftDraw = _leftEye;
    EyeParams rightDraw = _rightEye;
    
    if (!leftDraw.isHeart && !leftDraw.isStar && !leftDraw.isSpiral && !leftDraw.isCross && !leftDraw.isDollar && !leftDraw.isSquintV) {
        leftDraw.openness *= _blinkProgress;
    }
    if (_currentExpr != Expression::WINK && _currentExpr != Expression::PLAYFUL) {
        if (!rightDraw.isHeart && !rightDraw.isStar && !rightDraw.isSpiral && !rightDraw.isCross && !rightDraw.isDollar && !rightDraw.isSquintV) {
            rightDraw.openness *= _blinkProgress;
        }
    }
    
    // Apply idle look direction to pupils
    leftDraw.pupilX += _lookX;
    leftDraw.pupilY += _lookY;
    rightDraw.pupilX += _lookX;
    rightDraw.pupilY += _lookY;
    
    // Eyebrows
    if (_currentExpr == Expression::ANGRY || _currentExpr == Expression::DETERMINED) {
        d.drawLine(leftDraw.x - 12, leftDraw.y - 14, leftDraw.x + 12, leftDraw.y - 8, SSD1306_WHITE);
        d.drawLine(leftDraw.x - 12, leftDraw.y - 13, leftDraw.x + 12, leftDraw.y - 7, SSD1306_WHITE);
        d.drawLine(rightDraw.x - 12, rightDraw.y - 8, rightDraw.x + 12, rightDraw.y - 14, SSD1306_WHITE);
        d.drawLine(rightDraw.x - 12, rightDraw.y - 7, rightDraw.x + 12, rightDraw.y - 13, SSD1306_WHITE);
    }
    else if (_currentExpr == Expression::SKEPTICAL) {
        d.drawLine(leftDraw.x - 10, leftDraw.y - 14, leftDraw.x + 10, leftDraw.y - 12, SSD1306_WHITE);
        d.drawLine(leftDraw.x - 10, leftDraw.y - 13, leftDraw.x + 10, leftDraw.y - 11, SSD1306_WHITE);
    }
    else if (_currentExpr == Expression::SMUG) {
        // Raised arched smug eyebrows
        d.drawLine(leftDraw.x - 10, leftDraw.y - 14, leftDraw.x + 10, leftDraw.y - 15, SSD1306_WHITE);
        d.drawLine(rightDraw.x - 10, rightDraw.y - 15, rightDraw.x + 10, rightDraw.y - 12, SSD1306_WHITE);
    }
    
    // Draw eyes
    _drawEye(leftDraw, true);
    _drawEye(rightDraw, false);
    
    // Draw mouth
    _drawMouth(_mouth);
    
    // Draw extras
    _drawExtras(_extras);
    
    // Header: Floating Expression Toast Banner (when cycled via one-tap!)
    if (millis() - _toastStartTime < 1500) {
        int tw = 110;
        int tx = (SCREEN_W - tw) / 2;
        d.fillRect(tx, 1, tw, 11, SSD1306_BLACK);
        d.drawRoundRect(tx, 1, tw, 11, 2, SSD1306_WHITE);
        char toastBuf[32];
        snprintf(toastBuf, sizeof(toastBuf), "%d/30 %s", (int)_targetExpr + 1, getExpressionName(_targetExpr));
        _dm->drawCenteredText(toastBuf, 3, 1);
    }
}

void FaceRenderer::_drawEye(const EyeParams& eye, bool isLeft) {
    Adafruit_SSD1306& d = _dm->display();
    
    if (eye.isHeart) {
        float pulse = (eye.width > 20) ? sinf(_breathPhase * 2) * 1.5f : 0.0f;
        _drawHeartEye(eye.x, eye.y, (eye.width + pulse) * 0.5f);
        return;
    }
    if (eye.isStar) {
        _drawStarEye(eye.x, eye.y, eye.width * 0.5f);
        return;
    }
    if (eye.isSpiral) {
        _drawSpiralEye(eye.x, eye.y, eye.width * 0.5f);
        return;
    }
    if (eye.isCool) {
        _drawCoolEye(eye);
        return;
    }
    if (eye.isCross) {
        _drawCrossEye(eye.x, eye.y, eye.width * 0.5f);
        return;
    }
    if (eye.isDollar) {
        _drawDollarEye(eye.x, eye.y, eye.width * 0.5f);
        return;
    }
    if (eye.isPleading) {
        _drawPleadingEye(eye);
        return;
    }
    if (eye.isSquintV) {
        _drawSquintVEye(eye.x, eye.y, eye.width * 0.5f, isLeft);
        return;
    }
    
    float breathMod = sinf(_breathPhase) * 0.4f;
    float effectiveHeight = (eye.height + breathMod) * eye.openness;
    
    if (effectiveHeight < 1.5f) {
        // Cute sleeping eye: gentle breathing curved crescent with corner eyelash
        float sleepBreath = (_currentExpr == Expression::SLEEPING) ? (sinf(_breathPhase) * 1.2f) : 0.0f;
        int baseEyeY = (int)(eye.y + sleepBreath);
        int hw = (int)(eye.width / 2.0f);
        
        // Draw 2px thick smooth curved arc: dips gracefully in center
        for (int i = -hw; i <= hw; i++) {
            float t = (float)i / hw;
            int sag = (int)(3.5f * (1.0f - t * t));
            d.drawPixel((int)eye.x + i, baseEyeY + sag, SSD1306_WHITE);
            d.drawPixel((int)eye.x + i, baseEyeY + sag + 1, SSD1306_WHITE);
        }
        
        // Cute expressive corner eyelashes
        if (isLeft) {
            d.drawLine((int)eye.x - hw, baseEyeY, (int)eye.x - hw - 3, baseEyeY - 2, SSD1306_WHITE);
            d.drawLine((int)eye.x - hw, baseEyeY + 1, (int)eye.x - hw - 3, baseEyeY - 1, SSD1306_WHITE);
        } else {
            d.drawLine((int)eye.x + hw, baseEyeY, (int)eye.x + hw + 3, baseEyeY - 2, SSD1306_WHITE);
            d.drawLine((int)eye.x + hw, baseEyeY + 1, (int)eye.x + hw + 3, baseEyeY - 1, SSD1306_WHITE);
        }
        return;
    }
    
    float topReduction = eye.squint * effectiveHeight * 0.4f;
    float hw = eye.width / 2.0f;
    float hh = effectiveHeight / 2.0f;
    
    int drawX = (int)(eye.x - hw);
    int drawY = (int)(eye.y - hh + topReduction);
    int drawW = (int)eye.width;
    int drawH = (int)(effectiveHeight - topReduction);
    
    float px = eye.x + eye.pupilX * (hw - eye.pupilSize - 2);
    float py = eye.y + eye.pupilY * (hh - eye.pupilSize - 2);
    
    if (_eyeStyle == EyeStyle::VECTOR_SQUARE) {
        d.fillRoundRect(drawX, drawY, drawW, drawH, 4, SSD1306_WHITE);
        d.fillRoundRect((int)px - (int)eye.pupilSize, (int)py - (int)eye.pupilSize, (int)eye.pupilSize * 2, (int)eye.pupilSize * 2, 2, SSD1306_BLACK);
        d.fillRect((int)(px - 2), (int)(py - 2), 2, 2, SSD1306_WHITE);
        d.drawPixel((int)(px + 1), (int)(py + 1), SSD1306_WHITE);
    }
    else if (_eyeStyle == EyeStyle::NEON_OUTLINE) {
        d.drawRoundRect(drawX, drawY, drawW, drawH, 3, SSD1306_WHITE);
        d.drawRoundRect(drawX + 1, drawY + 1, drawW - 2, drawH - 2, 2, SSD1306_WHITE);
        d.fillCircle((int)px, (int)py, (int)eye.pupilSize, SSD1306_WHITE);
    }
    else if (_eyeStyle == EyeStyle::CYBER_DOT) {
        for (int rx = drawX; rx < drawX + drawW; rx += 3) {
            for (int ry = drawY; ry < drawY + drawH; ry += 3) {
                d.drawPixel(rx, ry, SSD1306_WHITE);
            }
        }
    }
    else {
        // CLASSIC_OVAL default
        int r = (int)(eye.roundness * min(hw, hh) * 0.5f);
        d.fillRoundRect(drawX, drawY, drawW, drawH, r > 0 ? r : 3, SSD1306_WHITE);
        d.fillCircle((int)px, (int)py, (int)eye.pupilSize, SSD1306_BLACK);
        d.fillRect((int)(px - 2), (int)(py - 2), 2, 2, SSD1306_WHITE);
        d.drawPixel((int)(px + 1), (int)(py + 1), SSD1306_WHITE);
    }
    
    // Nerd glasses outline
    if (eye.isNerd) {
        d.drawCircle((int)eye.x, (int)eye.y, (int)(eye.width * 0.65f), SSD1306_WHITE);
        if (isLeft) {
            d.drawFastHLine((int)(eye.x + eye.width * 0.65f), (int)eye.y, 16, SSD1306_WHITE);
        }
    }
}

void FaceRenderer::_drawCrossEye(float cx, float cy, float size) {
    Adafruit_SSD1306& d = _dm->display();
    int s = (int)size;
    d.drawLine((int)cx - s, (int)cy - s, (int)cx + s, (int)cy + s, SSD1306_WHITE);
    d.drawLine((int)cx - s, (int)cy - s + 1, (int)cx + s, (int)cy + s + 1, SSD1306_WHITE);
    d.drawLine((int)cx + s, (int)cy - s, (int)cx - s, (int)cy + s, SSD1306_WHITE);
    d.drawLine((int)cx + s, (int)cy - s + 1, (int)cx - s, (int)cy + s + 1, SSD1306_WHITE);
}

void FaceRenderer::_drawDollarEye(float cx, float cy, float size) {
    Adafruit_SSD1306& d = _dm->display();
    int s = (int)size;
    d.fillRoundRect((int)cx - s, (int)cy - s, s * 2, s * 2, 3, SSD1306_WHITE);
    d.setTextSize(2);
    d.setTextColor(SSD1306_BLACK);
    d.setCursor((int)cx - 5, (int)cy - 7);
    d.print("$");
    d.setTextColor(SSD1306_WHITE);
}

void FaceRenderer::_drawPleadingEye(const EyeParams& eye) {
    Adafruit_SSD1306& d = _dm->display();
    int w = (int)eye.width;
    int h = (int)eye.height;
    
    // Large filled rounded eye
    d.fillRoundRect((int)eye.x - w/2, (int)eye.y - h/2, w, h, 6, SSD1306_WHITE);
    // Huge dark pupil
    d.fillCircle((int)eye.x, (int)eye.y, w/2 - 2, SSD1306_BLACK);
    // Big top glint
    d.fillCircle((int)eye.x - 3, (int)eye.y - 3, 4, SSD1306_WHITE);
    // Secondary bottom glint
    d.fillCircle((int)eye.x + 4, (int)eye.y + 4, 2, SSD1306_WHITE);
}

void FaceRenderer::_drawSquintVEye(float cx, float cy, float size, bool isLeft) {
    Adafruit_SSD1306& d = _dm->display();
    int s = (int)size;
    if (isLeft) {
        // > shape
        d.drawLine((int)cx - s, (int)cy - s, (int)cx + s/2, (int)cy, SSD1306_WHITE);
        d.drawLine((int)cx - s, (int)cy - s + 1, (int)cx + s/2, (int)cy + 1, SSD1306_WHITE);
        d.drawLine((int)cx + s/2, (int)cy, (int)cx - s, (int)cy + s, SSD1306_WHITE);
        d.drawLine((int)cx + s/2, (int)cy + 1, (int)cx - s, (int)cy + s + 1, SSD1306_WHITE);
    } else {
        // < shape
        d.drawLine((int)cx + s, (int)cy - s, (int)cx - s/2, (int)cy, SSD1306_WHITE);
        d.drawLine((int)cx + s, (int)cy - s + 1, (int)cx - s/2, (int)cy + 1, SSD1306_WHITE);
        d.drawLine((int)cx - s/2, (int)cy, (int)cx + s, (int)cy + s, SSD1306_WHITE);
        d.drawLine((int)cx - s/2, (int)cy + 1, (int)cx + s, (int)cy + s + 1, SSD1306_WHITE);
    }
}

void FaceRenderer::_drawHeartEye(float cx, float cy, float size) {
    Adafruit_SSD1306& d = _dm->display();
    int s = (int)size;
    
    // Pixel-perfect bitmap for small / floating hearts
    if (s <= 8) {
        d.drawBitmap((int)cx - 4, (int)cy - 4, sym_heart_8, 8, 8, SSD1306_WHITE);
        return;
    }
    
    // Smooth, perfectly-proportioned heart for large eye expressions
    int r = (int)(s * 0.42f) + 1;
    int lobeOffset = r - 1;
    int lobeY = (int)cy - r / 2;
    
    // Two overlapping circular lobes at the top
    d.fillCircle((int)cx - lobeOffset, lobeY, r, SSD1306_WHITE);
    d.fillCircle((int)cx + lobeOffset, lobeY, r, SSD1306_WHITE);
    
    // Clean tangent V-shape tapering to bottom tip
    int sideX = lobeOffset + r;
    int midY = lobeY + r / 3;
    int bottomY = (int)cy + s + 1;
    
    d.fillTriangle(
        (int)cx - sideX, midY,
        (int)cx + sideX, midY,
        (int)cx, bottomY,
        SSD1306_WHITE
    );
    // Fill the central valley between the two circles
    d.fillRect((int)cx - lobeOffset, lobeY, lobeOffset * 2 + 1, midY - lobeY + 1, SSD1306_WHITE);
}

void FaceRenderer::_drawStarEye(float cx, float cy, float size) {
    Adafruit_SSD1306& d = _dm->display();
    int s = (int)size;
    d.fillTriangle((int)cx, (int)(cy - s), (int)(cx - 3), (int)cy, (int)(cx + 3), (int)cy, SSD1306_WHITE);
    d.fillTriangle((int)cx, (int)(cy + s), (int)(cx - 3), (int)cy, (int)(cx + 3), (int)cy, SSD1306_WHITE);
    d.fillTriangle((int)(cx - s), (int)cy, (int)cx, (int)(cy - 3), (int)cx, (int)(cy + 3), SSD1306_WHITE);
    d.fillTriangle((int)(cx + s), (int)cy, (int)cx, (int)(cy - 3), (int)cx, (int)(cy + 3), SSD1306_WHITE);
    d.fillCircle((int)cx, (int)cy, 3, SSD1306_WHITE);
}

void FaceRenderer::_drawSpiralEye(float cx, float cy, float size) {
    Adafruit_SSD1306& d = _dm->display();
    for (float a = 0; a < 4 * M_PI; a += 0.15f) {
        float r = (a / (4 * M_PI)) * size;
        float angle = a + _spiralAngle;
        int px = (int)(cx + r * cosf(angle));
        int py = (int)(cy + r * sinf(angle));
        d.drawPixel(px, py, SSD1306_WHITE);
        d.drawPixel(px + 1, py, SSD1306_WHITE);
    }
}

void FaceRenderer::_drawCoolEye(const EyeParams& eye) {
    Adafruit_SSD1306& d = _dm->display();
    int hw = (int)(eye.width / 2);
    int hh = (int)(eye.height / 2);
    d.fillRoundRect((int)(eye.x - hw), (int)(eye.y - hh), (int)eye.width, (int)eye.height, 3, SSD1306_WHITE);
    d.fillRoundRect((int)(eye.x - hw + 2), (int)(eye.y - hh + 2), (int)(eye.width - 4), (int)(eye.height - 4), 2, SSD1306_BLACK);
    d.drawLine((int)(eye.x - hw + 3), (int)(eye.y - hh + 3), (int)(eye.x - hw + 7), (int)(eye.y - hh + 3), SSD1306_WHITE);
    if (eye.x < 64) {
        d.drawLine((int)(eye.x + hw), (int)eye.y, (int)(eye.x + hw + 8), (int)eye.y, SSD1306_WHITE);
        d.drawLine((int)(eye.x + hw), (int)(eye.y - 1), (int)(eye.x + hw + 8), (int)(eye.y - 1), SSD1306_WHITE);
    }
}

void FaceRenderer::_drawMouth(const MouthParams& mouth) {
    Adafruit_SSD1306& d = _dm->display();
    int mx = (int)mouth.x;
    float sleepBreath = (_currentExpr == Expression::SLEEPING) ? (sinf(_breathPhase) * 1.2f) : 0.0f;
    int my = (int)(mouth.y + sleepBreath);
    int hw = (int)(mouth.width / 2);
    
    if (mouth.isCatSmirk) {
        // :3 cat mouth
        d.drawPixel(mx - 6, my - 2, SSD1306_WHITE);
        d.drawLine(mx - 6, my - 1, mx - 3, my + 1, SSD1306_WHITE);
        d.drawLine(mx - 3, my + 1, mx, my - 1, SSD1306_WHITE);
        d.drawLine(mx, my - 1, mx + 3, my + 1, SSD1306_WHITE);
        d.drawLine(mx + 3, my + 1, mx + 6, my - 1, SSD1306_WHITE);
        d.drawPixel(mx + 6, my - 2, SSD1306_WHITE);
        return;
    }
    
    if (mouth.isWavy) {
        for (int i = -hw; i <= hw; i++) {
            float wave = sinf(i * 0.5f) * 3;
            d.drawPixel(mx + i, my + (int)wave, SSD1306_WHITE);
            d.drawPixel(mx + i, my + (int)wave + 1, SSD1306_WHITE);
        }
        return;
    }
    
    if (mouth.isZigzag) {
        int segW = hw / 3;
        for (int i = 0; i < 6; i++) {
            int x1 = mx - hw + i * segW;
            int x2 = mx - hw + (i + 1) * segW;
            int y1 = my + ((i % 2 == 0) ? -2 : 2);
            int y2 = my + ((i % 2 == 0) ? 2 : -2);
            d.drawLine(x1, y1, x2, y2, SSD1306_WHITE);
            d.drawLine(x1, y1 + 1, x2, y2 + 1, SSD1306_WHITE);
        }
        return;
    }
    
    if (mouth.openness > 0.1f) {
        int mouthH = (int)(mouth.openness * 10);
        if (mouth.curve > 0.3f) {
            d.fillRoundRect(mx - hw, my - mouthH/2, hw * 2, mouthH, 4, SSD1306_WHITE);
            d.fillRect(mx - hw, my - mouthH/2 - 1, hw * 2, mouthH/2, SSD1306_BLACK);
            d.drawLine(mx - hw, my, mx + hw, my, SSD1306_WHITE);
        } else {
            d.drawRoundRect(mx - hw/2, my - mouthH/2, hw, mouthH, 3, SSD1306_WHITE);
            d.drawRoundRect(mx - hw/2 + 1, my - mouthH/2 + 1, hw - 2, mouthH - 2, 2, SSD1306_WHITE);
        }
    } else {
        if (abs(mouth.curve) < 0.05f) {
            d.drawLine(mx - hw, my, mx + hw, my, SSD1306_WHITE);
            d.drawLine(mx - hw, my + 1, mx + hw, my + 1, SSD1306_WHITE);
        } else {
            int curveH = (int)(mouth.curve * 8);
            for (int i = -hw; i <= hw; i++) {
                float t = (float)i / hw;
                float yOff = curveH * (1.0f - t * t);
                d.drawPixel(mx + i, my + (int)yOff, SSD1306_WHITE);
                d.drawPixel(mx + i, my + (int)yOff + 1, SSD1306_WHITE);
            }
        }
    }
    
    if (mouth.hasTongue) {
        d.fillRoundRect(mx - 3, my + 2, 8, 8, 3, SSD1306_WHITE);
        d.drawLine(mx + 1, my + 4, mx + 1, my + 9, SSD1306_BLACK);
    }
}

void FaceRenderer::_drawExtras(const FaceExtras& extras) {
    if (extras.showBlush) {
        _drawBlush(LEFT_EYE_X - 2, EYE_Y + 13, extras.blushIntensity);
        _drawBlush(RIGHT_EYE_X + 2, EYE_Y + 13, extras.blushIntensity);
        
        // Sweet floating affection hearts when Buddy is petted!
        if (_touchReacting) {
            unsigned long elapsed = millis() - _touchReactTime;
            // Primary floating heart
            float petPhase1 = (float)(elapsed % 1500) / 1500.0f;
            int heart1Y = (int)(EYE_Y - 8 - petPhase1 * 18.0f);
            if (heart1Y >= 2 && heart1Y <= 26) {
                _drawHeartEye(64.0f + sinf(petPhase1 * 6.28f) * 6.0f, heart1Y, 8);
            }
            
            // Secondary companion floating heart
            float petPhase2 = (float)((elapsed + 750) % 1500) / 1500.0f;
            int heart2Y = (int)(EYE_Y - 8 - petPhase2 * 18.0f);
            if (heart2Y >= 2 && heart2Y <= 26) {
                _drawHeartEye(54.0f - sinf(petPhase2 * 6.28f) * 5.0f, heart2Y, 8);
            }
        }
    }
    if (extras.showTear) {
        _drawTear(RIGHT_EYE_X + 11, EYE_Y + 6);
    }
    if (extras.showZzz) {
        _drawZzz(RIGHT_EYE_X + 16, EYE_Y - 14);
    }
    if (extras.showSweat) {
        _drawSweat(LEFT_EYE_X - 16, EYE_Y - 8);
    }
    if (extras.showMusic) {
        _drawMusicNote(RIGHT_EYE_X + 14, EYE_Y - 14);
    }
    if (extras.showExclaim) {
        Adafruit_SSD1306& d = _dm->display();
        d.fillRect(62, 2, 4, 8, SSD1306_WHITE);
        d.fillRect(62, 12, 4, 3, SSD1306_WHITE);
    }
}

void FaceRenderer::_drawBlush(float x, float y, float intensity) {
    Adafruit_SSD1306& d = _dm->display();
    int count = (int)(intensity * 4);
    for (int i = 0; i < count; i++) {
        int ox = i * 3 - (count * 3) / 2;
        d.drawLine((int)x + ox, (int)y, (int)x + ox + 2, (int)y + 2, SSD1306_WHITE);
    }
}

void FaceRenderer::_drawTear(float x, float y) {
    Adafruit_SSD1306& d = _dm->display();
    float phase = (float)(millis() % 2000) / 2000.0f;
    float dropY = y + phase * 14;
    d.fillCircle((int)x, (int)dropY + 2, 2, SSD1306_WHITE);
    d.fillTriangle((int)x, (int)dropY - 2, (int)x - 2, (int)dropY + 2, (int)x + 2, (int)dropY + 2, SSD1306_WHITE);
}

void FaceRenderer::_drawSweat(float x, float y) {
    Adafruit_SSD1306& d = _dm->display();
    d.fillCircle((int)x, (int)y + 3, 3, SSD1306_WHITE);
    d.fillTriangle((int)x, (int)y - 2, (int)x - 3, (int)y + 3, (int)x + 3, (int)y + 3, SSD1306_WHITE);
}

void FaceRenderer::_drawMusicNote(float x, float y) {
    Adafruit_SSD1306& d = _dm->display();
    float noteY = y + sinf(_musicPhase) * 3;
    d.fillCircle((int)x, (int)noteY + 4, 2, SSD1306_WHITE);
    d.drawFastVLine((int)x + 2, (int)noteY - 4, 8, SSD1306_WHITE);
    d.drawFastHLine((int)x + 2, (int)noteY - 4, 4, SSD1306_WHITE);
}

void FaceRenderer::_drawZzz(float x, float y) {
    Adafruit_SSD1306& d = _dm->display();
    
    // 1. Dream Sky: Cute Crescent Moon in top-left
    d.fillCircle(14, 11, 6, SSD1306_WHITE);
    d.fillCircle(17, 9, 5, SSD1306_BLACK); // Clean crescent cutout
    
    // 2. Twinkling Dream Stars
    int starPhase = (int)(_zzzPhase * 3.0f) % 6;
    if (starPhase != 0) {
        // Star 1 (Cross sparkle)
        d.drawPixel(27, 7, SSD1306_WHITE);
        d.drawPixel(26, 7, SSD1306_WHITE);
        d.drawPixel(28, 7, SSD1306_WHITE);
        d.drawPixel(27, 6, SSD1306_WHITE);
        d.drawPixel(27, 8, SSD1306_WHITE);
    }
    if (starPhase != 2) {
        // Star 2 (Cute side star)
        d.drawPixel(8, 24, SSD1306_WHITE);
        d.drawPixel(8, 23, SSD1306_WHITE);
        d.drawPixel(8, 25, SSD1306_WHITE);
        d.drawPixel(7, 24, SSD1306_WHITE);
        d.drawPixel(9, 24, SSD1306_WHITE);
    }
    if (starPhase != 4) {
        // Star 3 (Distant twinkle)
        d.drawPixel(36, 14, SSD1306_WHITE);
    }
    
    // 3. Floating, Undulating Zzz Dream Bubbles
    for (int i = 0; i < 3; i++) {
        float offset = (float)i / 3.0f;
        float progress = fmodf((_zzzPhase / (2.0f * M_PI)) + offset, 1.0f);
        
        // Gentle wave drift path rising towards upper-right
        float curX = x + progress * 14.0f + sinf(progress * 6.28f + i) * 3.5f;
        float curY = (y + 12.0f) - progress * 28.0f;
        
        if (curY < 2 || curY > 58 || curX < 0 || curX > 124) continue;
        
        int zx = (int)curX;
        int zy = (int)curY;
        
        if (progress < 0.35f) {
            // Smallest cute 'z' (4x4)
            d.drawFastHLine(zx, zy, 4, SSD1306_WHITE);
            d.drawLine(zx + 3, zy, zx, zy + 3, SSD1306_WHITE);
            d.drawFastHLine(zx, zy + 3, 4, SSD1306_WHITE);
        } else if (progress < 0.70f) {
            // Medium 'Z' (6x6)
            d.drawFastHLine(zx, zy, 6, SSD1306_WHITE);
            d.drawLine(zx + 5, zy, zx, zy + 5, SSD1306_WHITE);
            d.drawFastHLine(zx, zy + 5, 6, SSD1306_WHITE);
        } else {
            // Large Bold 'Z' (8x8 near top of dream)
            d.drawFastHLine(zx, zy, 8, SSD1306_WHITE);
            d.drawFastHLine(zx, zy + 1, 8, SSD1306_WHITE);
            d.drawLine(zx + 6, zy, zx + 1, zy + 6, SSD1306_WHITE);
            d.drawLine(zx + 7, zy, zx + 2, zy + 6, SSD1306_WHITE);
            d.drawFastHLine(zx, zy + 6, 8, SSD1306_WHITE);
            d.drawFastHLine(zx, zy + 7, 8, SSD1306_WHITE);
        }
    }
}
