#pragma once
// ============================================================================
// DeskBuddy — Touch Input System
// ============================================================================
#include <Arduino.h>
#include "config.h"

enum class TouchEvent {
    NONE,
    TOUCH_DOWN,     // Immediate press down (0ms latency for arcade games & instant response)
    TAP,            // Quick press < 300ms
    DOUBLE_TAP,     // Two taps within gap
    TRIPLE_TAP,     // Three taps in rapid succession
    LONG_PRESS,     // Hold 600ms-1.6s
    HOLD,           // Hold 1.6s-4s
    DEEP_HOLD       // Hold > 4s
};

// Callback function type
typedef void (*TouchCallback)(TouchEvent event);

class TouchInput {
public:
    void begin();
    TouchEvent update();  // Call every frame; returns event if one occurred
    
    // State queries
    bool isTouching() const { return _touching; }
    unsigned long touchDuration() const;
    unsigned long timeSinceLastTouch() const;
    
    // Register callback (optional)
    void setCallback(TouchCallback cb) { _callback = cb; }
    
private:
    bool _touching = false;
    bool _lastRawReading = false;
    bool _debouncedState = false;
    
    unsigned long _touchStart = 0;
    unsigned long _touchEnd = 0;
    unsigned long _lastDebounceTime = 0;
    
    // Tap detection state machine
    enum class TapState {
        IDLE,
        FIRST_DOWN,
        FIRST_UP_WAITING,   // waiting to see if double-tap
        SECOND_DOWN,
        SECOND_UP_WAITING,  // waiting to see if triple-tap
        THIRD_DOWN,
        LONG_PRESSING,
        HOLDING,
        DEEP_HOLDING
    };
    
    TapState _tapState = TapState::IDLE;
    unsigned long _firstTapTime = 0;
    unsigned long _secondTapTime = 0;
    bool _longPressFired = false;
    bool _holdFired = false;
    bool _deepHoldFired = false;
    
    TouchCallback _callback = nullptr;
    
    TouchEvent _emitEvent(TouchEvent evt);
};
