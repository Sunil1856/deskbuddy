#pragma once
// ============================================================================
// DeskBuddy — Coin Toss Game (Heads or Tails)
// ============================================================================
#include <Arduino.h>
#include "display_manager.h"
#include "storage.h"
#include "face_renderer.h"
#include "touch_input.h"

enum class TossResult {
    NONE,
    HEADS,
    TAILS
};

class GameToss {
public:
    GameToss(DisplayManager* dm, Storage* storage, FaceRenderer* face);
    
    void onEnter();
    void update();
    void render();
    void handleTouch(TouchEvent event);

private:
    DisplayManager* _dm = nullptr;
    Storage* _storage = nullptr;
    FaceRenderer* _face = nullptr;
    
    enum class State {
        READY,
        FLIPPING,
        RESULT
    };
    
    State _state = State::READY;
    TossResult _result = TossResult::NONE;
    
    // Animation variables
    unsigned long _flipStart = 0;
    const unsigned long FLIP_DURATION_MS = 1100;
    float _spinAngle = 0.0f;
    
    // Sparkle animation particles
    struct Sparkle {
        float x, y;
        float vx, vy;
        uint8_t life;
    };
    static const int MAX_SPARKLES = 6;
    Sparkle _sparkles[MAX_SPARKLES];
    
    // Session tally
    uint16_t _headsCount = 0;
    uint16_t _tailsCount = 0;
    uint8_t _currentStreak = 0;
    TossResult _streakType = TossResult::NONE;
    
    void _startFlip();
    void _finishFlip();
    void _drawCoin3D(int cx, int cy, float heightFactor, bool isHeads);
    void _drawResultCoin(int cx, int cy, TossResult res);
};
