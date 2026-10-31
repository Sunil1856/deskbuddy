#pragma once
// ============================================================================
// DeskBuddy — Multi-Page Message & AI Response Reader
// ============================================================================
#include <Arduino.h>
#include <vector>
#include "display_manager.h"
#include "face_renderer.h"
#include "touch_input.h"

class AppMessage {
public:
    AppMessage();
    void begin(DisplayManager* dm, FaceRenderer* face);
    
    void onEnter();
    void onExit();
    void update();
    void render();
    void handleTouch(TouchEvent event);
    
    // Set new message (automatically formats and splits into pages)
    void setMessage(const String& text, const String& emotion = "HAPPY", const String& title = "BUDDY AI");
    
    int getCurrentPage() const { return _currentPage; }
    int getTotalPages() const { return _totalPages; }
    bool hasMessage() const { return !_lines.empty(); }
    bool shouldExit() const { return _shouldExit; }
    void clearExit() { _shouldExit = false; }
    
private:
    DisplayManager* _dm = nullptr;
    FaceRenderer* _face = nullptr;
    
    String _title = "BUDDY AI";
    String _emotion = "HAPPY";
    String _rawText = "";
    
    std::vector<String> _lines;
    int _currentPage = 0;
    int _totalPages = 1;
    bool _shouldExit = false;
    
    unsigned long _lastInteractionTime = 0;
    static const int LINES_PER_PAGE = 4;
    static const unsigned long AUTO_EXIT_TIMEOUT_MS = 60000; // 60s auto-exit back to Face
};
