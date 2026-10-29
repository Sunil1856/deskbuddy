#pragma once
// ============================================================================
// DeskBuddy — Magic Oracle (Cyber 8-Ball & Decision Maker)
// ============================================================================
#include <Arduino.h>
#include "display_manager.h"
#include "storage.h"
#include "face_renderer.h"
#include "touch_input.h"

class GameOracle {
public:
    GameOracle(DisplayManager* dm, Storage* storage, FaceRenderer* face);
    
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
        THINKING,
        REVEAL
    };
    
    State _state = State::READY;
    
    // Animation variables
    unsigned long _thinkStart = 0;
    const unsigned long THINK_DURATION_MS = 1250;
    float _shakeX = 0.0f;
    float _shakeY = 0.0f;
    float _vortexAngle = 0.0f;
    
    // Cosmic particle effect
    struct CosmicParticle {
        float angle;
        float dist;
        float speed;
        uint8_t life;
    };
    static const int MAX_PARTICLES = 10;
    CosmicParticle _particles[MAX_PARTICLES];
    
    // Answer selection
    int _answerIndex = 0;
    
    void _startConsultation();
    void _revealFortune();
    void _draw8Ball(int cx, int cy, int radius);
    void _drawTriangleDie(int cx, int cy);
};
