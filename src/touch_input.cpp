// ============================================================================
// DeskBuddy — Touch Input System Implementation
// ============================================================================
#include "touch_input.h"

void TouchInput::begin() {
    pinMode(PIN_TOUCH, INPUT_PULLDOWN);
    _tapState = TapState::IDLE;
    _touching = false;
    _lastRawReading = false;
    _debouncedState = false;
    _touchStart = 0;
    _touchEnd = 0;
    _lastDebounceTime = 0;
    _firstTapTime = 0;
    _secondTapTime = 0;
}

unsigned long TouchInput::touchDuration() const {
    if (_touching) {
        return millis() - _touchStart;
    }
    return 0;
}

unsigned long TouchInput::timeSinceLastTouch() const {
    if (_touching) return 0;
    if (_touchEnd == 0) return millis(); // never touched
    return millis() - _touchEnd;
}

TouchEvent TouchInput::_emitEvent(TouchEvent evt) {
    if (evt != TouchEvent::NONE) {
        const char* evtNames[] = {"NONE", "DOWN", "TAP", "DOUBLE_TAP", "TRIPLE_TAP", "LONG_PRESS", "HOLD", "DEEP_HOLD"};
        Serial.printf("[TOUCH] Event: %s\n", evtNames[(int)evt]);
    }
    if (_callback && evt != TouchEvent::NONE) {
        _callback(evt);
    }
    return evt;
}

TouchEvent TouchInput::update() {
    unsigned long now = millis();
    
    // Read digital pin from TTP223 touch sensor
    bool raw = (digitalRead(PIN_TOUCH) == HIGH);
    
    // Responsive steady-state integration debounce
    if (raw != _lastRawReading) {
        _lastRawReading = raw;
        _lastDebounceTime = now;
    }
    
    if ((now - _lastDebounceTime) >= TOUCH_DEBOUNCE_MS) {
        _debouncedState = raw;
    }

    // Stuck-Sensor Watchdog Protection:
    // If the sensor is stuck HIGH continuously for > TOUCH_STUCK_TIMEOUT_MS (e.g. electrical noise / static charge),
    // cleanly auto-release the state so the system and UI never freeze!
    if (_touching && (now - _touchStart) >= TOUCH_STUCK_TIMEOUT_MS) {
        Serial.println("[TOUCH] Watchdog: stuck touch released automatically");
        _touching = false;
        _debouncedState = false;
        _tapState = TapState::IDLE;
        _touchEnd = now;
        _lastDebounceTime = now;
        _longPressFired = false;
        _holdFired = false;
        _deepHoldFired = false;
        return TouchEvent::NONE;
    }
    
    bool pressed = _debouncedState;
    bool justPressed = pressed && !_touching;
    bool justReleased = !pressed && _touching;
    
    if (justPressed) {
        _touching = true;
        _touchStart = now;
        _longPressFired = false;
        _holdFired = false;
        _deepHoldFired = false;
        Serial.println("[TOUCH] Down");
    }
    
    if (justReleased) {
        _touching = false;
        _touchEnd = now;
        Serial.printf("[TOUCH] Up (held %lu ms)\n", _touchEnd - _touchStart);
    }
    
    // State machine for gesture detection
    switch (_tapState) {
        case TapState::IDLE:
            if (justPressed) {
                _tapState = TapState::FIRST_DOWN;
                return _emitEvent(TouchEvent::TOUCH_DOWN);
            }
            break;
            
        case TapState::FIRST_DOWN:
            if (justReleased) {
                unsigned long pressDur = _touchEnd - _touchStart;
                if (pressDur < TOUCH_LONG_PRESS_MS) {
                    // Short tap — wait to see if it's a double-tap
                    _firstTapTime = _touchEnd;
                    _tapState = TapState::FIRST_UP_WAITING;
                } else {
                    // Was held too long for a tap, already handled below
                    _tapState = TapState::IDLE;
                }
            } else if (pressed) {
                unsigned long held = now - _touchStart;
                if (held >= TOUCH_DEEP_HOLD_MS && !_deepHoldFired) {
                    _deepHoldFired = true;
                    _tapState = TapState::DEEP_HOLDING;
                    return _emitEvent(TouchEvent::DEEP_HOLD);
                } else if (held >= TOUCH_HOLD_MS && !_holdFired) {
                    _holdFired = true;
                    _tapState = TapState::HOLDING;
                    return _emitEvent(TouchEvent::HOLD);
                } else if (held >= TOUCH_LONG_PRESS_MS && !_longPressFired) {
                    _longPressFired = true;
                    _tapState = TapState::LONG_PRESSING;
                    return _emitEvent(TouchEvent::LONG_PRESS);
                }
            }
            break;
            
        case TapState::FIRST_UP_WAITING:
            if (justPressed) {
                // Second tap started!
                _tapState = TapState::SECOND_DOWN;
                return _emitEvent(TouchEvent::TOUCH_DOWN);
            } else if ((now - _firstTapTime) > TOUCH_DOUBLE_GAP_MS) {
                // Timed out waiting for second tap → single tap
                _tapState = TapState::IDLE;
                return _emitEvent(TouchEvent::TAP);
            }
            break;
            
        case TapState::SECOND_DOWN:
            if (justReleased) {
                unsigned long pressDur = _touchEnd - _touchStart;
                if (pressDur < TOUCH_LONG_PRESS_MS) {
                    // Second tap released quickly → wait to see if it's a triple-tap!
                    _secondTapTime = _touchEnd;
                    _tapState = TapState::SECOND_UP_WAITING;
                } else {
                    _tapState = TapState::IDLE;
                    return _emitEvent(TouchEvent::TAP); // first tap still counts
                }
            } else if (pressed) {
                unsigned long held = now - _touchStart;
                if (held >= TOUCH_LONG_PRESS_MS) {
                    // Held on second press — treat first as tap, this as long
                    _tapState = TapState::IDLE;
                    return _emitEvent(TouchEvent::TAP);
                }
            }
            break;
            
        case TapState::SECOND_UP_WAITING:
            if (justPressed) {
                // Third tap started!
                _tapState = TapState::THIRD_DOWN;
                return _emitEvent(TouchEvent::TOUCH_DOWN);
            } else if ((now - _secondTapTime) > TOUCH_DOUBLE_GAP_MS) {
                // Timed out waiting for third tap → it's a double tap!
                _tapState = TapState::IDLE;
                return _emitEvent(TouchEvent::DOUBLE_TAP);
            }
            break;
            
        case TapState::THIRD_DOWN:
            if (justReleased) {
                unsigned long pressDur = _touchEnd - _touchStart;
                if (pressDur < TOUCH_LONG_PRESS_MS) {
                    _tapState = TapState::IDLE;
                    return _emitEvent(TouchEvent::TRIPLE_TAP);
                } else {
                    _tapState = TapState::IDLE;
                    return _emitEvent(TouchEvent::DOUBLE_TAP);
                }
            } else if (pressed) {
                unsigned long held = now - _touchStart;
                if (held >= TOUCH_LONG_PRESS_MS) {
                    _tapState = TapState::IDLE;
                    return _emitEvent(TouchEvent::DOUBLE_TAP);
                }
            }
            break;
            
        case TapState::LONG_PRESSING:
            if (justReleased) {
                _tapState = TapState::IDLE;
            } else if (pressed) {
                unsigned long held = now - _touchStart;
                if (held >= TOUCH_DEEP_HOLD_MS && !_deepHoldFired) {
                    _deepHoldFired = true;
                    _tapState = TapState::DEEP_HOLDING;
                    return _emitEvent(TouchEvent::DEEP_HOLD);
                } else if (held >= TOUCH_HOLD_MS && !_holdFired) {
                    _holdFired = true;
                    _tapState = TapState::HOLDING;
                    return _emitEvent(TouchEvent::HOLD);
                }
            }
            break;
            
        case TapState::HOLDING:
            if (justReleased) {
                _tapState = TapState::IDLE;
            } else if (pressed) {
                unsigned long held = now - _touchStart;
                if (held >= TOUCH_DEEP_HOLD_MS && !_deepHoldFired) {
                    _deepHoldFired = true;
                    _tapState = TapState::DEEP_HOLDING;
                    return _emitEvent(TouchEvent::DEEP_HOLD);
                }
            }
            break;
            
        case TapState::DEEP_HOLDING:
            if (justReleased) {
                _tapState = TapState::IDLE;
            }
            break;
    }
    
    return TouchEvent::NONE;
}
