#pragma once
// ============================================================================
// DeskBuddy — Real-Time Voice AI Assistant App
// ============================================================================
#include <Arduino.h>
#include "display_manager.h"
#include "storage.h"
#include "face_renderer.h"
#include "touch_input.h"
#include "audio_i2s.h"

enum class VoiceState {
    IDLE,
    LISTENING,
    THINKING,
    SPEAKING
};

class AppVoice {
public:
    AppVoice();
    ~AppVoice();

    void begin(DisplayManager* dm, Storage* storage, FaceRenderer* face, AudioI2S* audio);
    void onEnter();
    void onExit();
    void update();
    void render();
    void handleTouch(TouchEvent event);

    void startListening();
    void stopListeningAndSend();
    void cancel();

    VoiceState getState() const { return _state; }

private:
    DisplayManager* _dm = nullptr;
    Storage* _storage = nullptr;
    FaceRenderer* _face = nullptr;
    AudioI2S* _audio = nullptr;

    VoiceState _state = VoiceState::IDLE;

    // Real-Time Recording Buffer (up to 3.5 seconds @ 16kHz 16-bit mono = ~112 KB)
    // Dynamic allocation on enter to preserve heap when not in voice app
    int16_t* _recBuffer = nullptr;
    size_t _maxSamples = 16000 * 3.5; // ~3.5s
    size_t _recordedSamples = 0;

    // Voice Activity Detection (VAD)
    bool _isSpeechDetected = false;
    unsigned long _speechStartTime = 0;
    unsigned long _lastSpeechTime = 0;
    unsigned long _listeningStartTime = 0;
    float _currentRms = 0.0f;
    float _smoothedRms = 0.0f;

    // UI & Waveform Visualizer
    uint8_t _waveHistory[16];
    uint8_t _animFrame = 0;
    unsigned long _lastAnimTime = 0;

    // Response Data
    String _lastReply = "";
    String _lastEmotion = "HAPPY";
    String _audioUrl = "";
    unsigned long _stateStartTime = 0;

    // Methods
    void _processListening();
    void _sendAudioToServer();
    void _playAudioStream(const String& url);

    void _renderListening();
    void _renderThinking();
    void _renderSpeaking();
    void _renderIdle();

    String _getServerUrl();
};
