#pragma once
// ============================================================================
// DeskBuddy — Face Renderer (Procedural Expressive Face Engine)
// ============================================================================
#include <Arduino.h>
#include "display_manager.h"

class Storage;

enum class EyeStyle {
    CLASSIC_OVAL,
    VECTOR_SQUARE,
    NEON_OUTLINE,
    CYBER_DOT,
    COUNT
};

// 30 Unique procedural animated expressions!
enum class Expression {
    NEUTRAL,
    HAPPY,
    LOVE,
    SLEEPY,
    SLEEPING,
    SURPRISED,
    ANGRY,
    SAD,
    CONFUSED,
    EXCITED,
    WINK,
    DIZZY,
    COOL,
    BLUSHING,
    SKEPTICAL,
    STARRY,
    PLAYFUL,
    CHILL,
    PARTY,
    SHOCKED,
    SMUG,
    PLEADING,
    NERD,
    SWEAT,
    MONEY,
    DEAD,
    DETERMINED,
    SINGING,
    HEARTBEAT,
    LAUGHING,
    COUNT  // total = 30
};

// Eye parameters (fully parametric)
struct EyeParams {
    float x, y;           // center position
    float width, height;  // ellipse dimensions
    float pupilX, pupilY; // pupil offset from center (-1 to 1)
    float pupilSize;      // pupil radius
    float openness;       // 0.0 = closed, 1.0 = fully open
    float squint;         // 0.0 = none, 1.0 = full squint
    float roundness;      // corner radius factor
    bool isHeart;         // draw as heart shape
    bool isStar;          // draw as star shape
    bool isSpiral;        // draw as spiral (dizzy)
    bool isCool;          // draw as rectangle (sunglasses)
    bool isCross;         // draw as X (KO/dead)
    bool isDollar;        // draw as $ (money)
    bool isPleading;      // draw as giant watery puppy eyes
    bool isNerd;          // draw glasses
    bool isSquintV;       // draw as squinting > < (laughing)
};

// Mouth parameters
struct MouthParams {
    float x, y;           // center position
    float width;          // mouth width
    float curve;          // -1.0 = frown, 0.0 = flat, 1.0 = smile
    float openness;       // 0.0 = closed line, 1.0 = fully open
    bool isWavy;          // confused wavy mouth
    bool isZigzag;        // angry zigzag mouth
    bool hasTongue;       // tongue sticking out
    bool isCatSmirk;      // :3 cat smirk
};

// Face extras
struct FaceExtras {
    bool showBlush;       // blush marks on cheeks
    bool showTear;        // tear drop
    bool showZzz;         // sleeping Z's
    bool showExclaim;     // exclamation mark
    bool showSweat;       // anime sweat drop
    bool showMusic;       // musical note
    float blushIntensity; // 0.0 - 1.0
};

class FaceRenderer {
public:
    void begin(DisplayManager* dm, Storage* storage = nullptr);
    void update();         // call every frame
    void render();         // draw current face state
    
    // Expression & Style control
    void setExpression(Expression expr, uint16_t transitionMs = EXPRESSION_TRANSITION_MS);
    void cycleNextExpression();
    Expression getExpression() const { return _targetExpr; }
    const char* getExpressionName(Expression expr) const;
    
    void setEyeStyle(EyeStyle style) { _eyeStyle = style; }
    EyeStyle getEyeStyle() const { return _eyeStyle; }
    
    // Emotional Mood System
    uint8_t getHappiness() const { return _happiness; }
    void setHappiness(uint8_t val);
    void addHappiness(int delta);
    void updateMood();
    
    // External triggers
    void onTouch();        // react to touch
    void onLongTouch();    // react to sustained touch
    void onWifiConnect();  // excited expression
    void onWifiFail();     // confused expression
    void onGameOver();     // dizzy expression
    void onGoodWeather();  // cool expression
    
    // Idle management
    void resetIdleTimer();
    void goToSleep();
    void wakeUp();
    bool isSleeping() const { return _isIdleSleeping || _currentExpr == Expression::SLEEPING; }
    
private:
    DisplayManager* _dm = nullptr;
    Storage* _storage = nullptr;
    uint8_t _happiness = 75;
    
    // Current and target state
    EyeStyle _eyeStyle = EyeStyle::CLASSIC_OVAL;
    Expression _currentExpr = Expression::NEUTRAL;
    Expression _targetExpr = Expression::NEUTRAL;
    bool _manualHold = false;
    
    // Animation & Micro-expression state
    float _breathPhase = 0.0f;
    float _transProgress = 1.0f;
    uint16_t _transDuration = 300;
    unsigned long _transStartTime = 0;
    unsigned long _toastStartTime = 0;
    
    // Eye parameters (interpolated)
    EyeParams _leftEye, _rightEye;
    EyeParams _targetLeftEye, _targetRightEye;
    MouthParams _mouth;
    MouthParams _targetMouth;
    FaceExtras _extras;
    FaceExtras _targetExtras;
    
    // Blink system
    unsigned long _nextBlinkTime = 0;
    float _blinkProgress = 1.0f;
    bool _blinkClosing = false;
    unsigned long _blinkStartTime = 0;
    bool _doDoubleBlink = false;
    int _doubleBlinkCount = 0;
    
    // Idle look-around
    unsigned long _nextLookTime = 0;
    float _lookX = 0, _lookY = 0;
    float _targetLookX = 0, _targetLookY = 0;
    unsigned long _lookTransStart = 0;
    
    // Idle timeout
    unsigned long _lastActivityTime = 0;
    bool _isIdleSleepy = false;
    bool _isIdleSleeping = false;
    
    // Floating animations
    float _zzzPhase = 0;
    float _musicPhase = 0;
    float _spiralAngle = 0;
    
    // Touch reaction
    unsigned long _touchReactTime = 0;
    bool _touchReacting = false;
    int _touchCount = 0;
    unsigned long _lastTouchTime = 0;
    
    // Spontaneous autonomous pet life expressions
    unsigned long _nextSpontaneousExprTime = 0;
    bool _isSpontaneousActive = false;
    unsigned long _spontaneousEndTime = 0;
    
    // Internal methods
    void _setExpressionParams(Expression expr, EyeParams& le, EyeParams& re, MouthParams& m, FaceExtras& ex);
    void _interpolateParams(float t);
    void _updateBlink();
    void _updateIdleLook();
    void _updateIdleTimeout();
    void _updateZzz();
    
    // Drawing
    void _drawEye(const EyeParams& eye, bool isLeft);
    void _drawHeartEye(float cx, float cy, float size);
    void _drawStarEye(float cx, float cy, float size);
    void _drawSpiralEye(float cx, float cy, float size);
    void _drawCoolEye(const EyeParams& eye);
    void _drawCrossEye(float cx, float cy, float size);
    void _drawDollarEye(float cx, float cy, float size);
    void _drawPleadingEye(const EyeParams& eye);
    void _drawSquintVEye(float cx, float cy, float size, bool isLeft);
    void _drawMouth(const MouthParams& mouth);
    void _drawExtras(const FaceExtras& extras);
    void _drawBlush(float x, float y, float intensity);
    void _drawTear(float x, float y);
    void _drawZzz(float x, float y);
    void _drawSweat(float x, float y);
    void _drawMusicNote(float x, float y);
};
