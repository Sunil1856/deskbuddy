// ============================================================================
// DeskBuddy — Main Application Entry
// ============================================================================
#include <Arduino.h>
#include "config.h"
#include "display_manager.h"
#include "touch_input.h"
#include "storage.h"
#include "face_renderer.h"
#include "menu_system.h"
#include "wifi_manager.h"
#include "time_service.h"

// Apps
#include "app_clock.h"
#include "app_weather.h"
#include "app_world_info.h"
#include "app_server.h"
#include "app_games.h"
#include "app_settings.h"
#include "app_gallery.h"
#include "app_message.h"

// Core subsystems
DisplayManager  g_display;
TouchInput      g_touchInput;
Storage         g_storage;
FaceRenderer    g_face;
MenuSystem      g_menu;
WifiManager     g_wifi;
TimeService     g_time;

// Apps
AppClock        appClock;
AppWeather      appWeather;
AppWorldInfo    appWorldInfo;
AppServer       appServer;
AppGames        appGames;
AppSettings     appSettings;
AppGallery      appGallery;
AppMessage      appMessage;

// Current state
AppID g_currentApp = AppID::FACE;

// Forward declaration
void switchApp(AppID newApp);

void setup() {
    Serial.begin(115200);
    delay(500); // Give serial time to attach
    
    Serial.println("DeskBuddy Starting...");
    
    // Init subsystems
    g_storage.begin();
    
    if (!g_display.begin()) {
        Serial.println("OLED init failed!");
        while (1) delay(100);
    }
    
    // Apply initial settings
    g_display.setBrightness(g_storage.getBrightness());
    g_display.setFlip(g_storage.getDisplayFlip());
    
    g_touchInput.begin();
    g_time.begin(&g_storage);
    g_face.begin(&g_display, &g_storage);
    g_face.setEyeStyle((EyeStyle)g_storage.getEyeStyle());
    
    g_menu.begin(&g_display);
    g_wifi.begin(&g_display, &g_storage);
    
    // Init apps
    appClock.begin(&g_display, &g_time, &g_storage);
    appWeather.begin(&g_display, &g_storage, &g_face);
    appWorldInfo.begin(&g_display, &g_storage);
    appServer.begin(&g_display, &g_storage);
    appGames.begin(&g_display, &g_storage, &g_face);
    appGallery.begin(&g_display, &g_storage, &g_face);
    appSettings.begin(&g_display, &g_storage, &g_wifi, &g_face);
    appMessage.begin(&g_display, &g_face);
    
    // Connect Photo Studio
    g_wifi.setGallery(&appGallery);
    g_wifi.setOnPhotoUploaded([]() {
        if (g_currentApp != AppID::GALLERY) {
            switchApp(AppID::GALLERY);
        }
    });

    // Connect Message & AI Chat reader
    g_wifi.setMessageApp(&appMessage);
    g_wifi.setOnMessageReceived([]() {
        if (g_currentApp != AppID::MESSAGE) {
            switchApp(AppID::MESSAGE);
        }
    });
    
    // Check if configured
    if (!g_storage.isConfigured()) {
        g_wifi.startPortal();
        return; // Stay in portal until configured & restarted
    }
    
    g_wifi.autoConnect();
}

static unsigned long g_lastAppSwitchTime = 0;

void switchApp(AppID newApp) {
    if (g_currentApp == newApp) return;
    
    // Clear residual touch state on app switch
    g_touchInput.reset();
    g_lastAppSwitchTime = millis();
    
    // Capture current frame buffer before transition so fade doesn't flash black/garbage
    g_display.captureScreen();
    
    // Exit current
    switch (g_currentApp) {
        case AppID::CLOCK: appClock.onExit(); break;
        case AppID::WEATHER: appWeather.onExit(); break;
        case AppID::WORLD_INFO: appWorldInfo.onExit(); break;
        case AppID::SERVER: appServer.onExit(); break;
        case AppID::GAMES: appGames.onExit(); break;
        case AppID::GALLERY: appGallery.onExit(); break;
        case AppID::SETTINGS: appSettings.onExit(); break;
        case AppID::MESSAGE: appMessage.onExit(); break;
        default: break;
    }
    
    g_currentApp = newApp;
    g_display.startTransition(Transition::FADE, 250);
    
    // Enter new
    switch (g_currentApp) {
        case AppID::FACE: break;
        case AppID::CLOCK: appClock.onEnter(); break;
        case AppID::WEATHER: appWeather.onEnter(); break;
        case AppID::WORLD_INFO: appWorldInfo.onEnter(); break;
        case AppID::SERVER: appServer.onEnter(); break;
        case AppID::GAMES: appGames.onEnter(); break;
        case AppID::GALLERY: appGallery.onEnter(); break;
        case AppID::SETTINGS: appSettings.onEnter(); break;
        case AppID::MESSAGE: appMessage.onEnter(); break;
        default: break;
    }
}

void loop() {
    // 1. Update core services
    g_wifi.update();
    g_time.update();
    
    if (g_wifi.getState() == WifiState::PORTAL_ACTIVE) {
        // Portal takes over the loop entirely
        return;
    }
    
    // 2. Periodic background tasks
    static unsigned long lastBackground = 0;
    if (millis() - lastBackground > 60000) { // Every minute
        appWeather.backgroundUpdate();
        lastBackground = millis();
    }
    
    // User touch inactivity tracker
    static unsigned long g_lastUserActivityTime = millis();
    
    // Auto-cycle tabs (configured via Settings: OFF / 1m / 2m / 3m / 5m)
    static unsigned long lastAutoCycle = millis();
    uint8_t autoCycleMode = g_storage.getAutoCycle();
    unsigned long cycleIntervalMs = 0;
    if (autoCycleMode == 1) cycleIntervalMs = 60000UL;
    else if (autoCycleMode == 2) cycleIntervalMs = 120000UL;
    else if (autoCycleMode == 3) cycleIntervalMs = 180000UL; // 3 min
    else if (autoCycleMode == 4) cycleIntervalMs = 300000UL; // 5 min
    
    // Auto-exit Settings after 2 min of inactivity: return to cute Face and resume Auto-Cycle
    if (g_currentApp == AppID::SETTINGS && (millis() - g_lastUserActivityTime > 120000UL)) {
        switchApp(AppID::FACE);
        lastAutoCycle = millis();
    }
    
    // Auto-exit Message reader after timeout: return to Pet Face
    if (g_currentApp == AppID::MESSAGE && appMessage.shouldExit()) {
        appMessage.clearExit();
        switchApp(AppID::FACE);
        lastAutoCycle = millis();
    }
    
    // Global User Inactivity Sleep Manager (Setting #8: Sleep in - 10 to 15 min default)
    uint32_t sleepTimeoutMs = g_storage.getSleepTimeout();
    if (sleepTimeoutMs > 0 && !g_face.isSleeping() && (millis() - g_lastUserActivityTime >= sleepTimeoutMs)) {
        // Active Tasks Protection: NEVER put to sleep if games is open or clock is running stopwatch/timer!
        if (g_currentApp != AppID::GAMES && !appGames.isGameActive() && !appClock.isBusy()) {
            if (g_menu.isActive()) {
                g_menu.hide();
                g_menu.clearSelectedApp();
            }
            if (g_currentApp != AppID::FACE) {
                switchApp(AppID::FACE);
            }
            g_face.goToSleep();
        }
    }
    
    if (cycleIntervalMs > 0 && millis() - lastAutoCycle > cycleIntervalMs) {
        lastAutoCycle = millis();
        // Do not auto-cycle away if menu is open, in games, in settings, or if clock is actively timing, or if Buddy is sleeping, or if actively watching a video in Gallery!
        if (!g_menu.isActive() && g_currentApp != AppID::GAMES && g_currentApp != AppID::SETTINGS && g_currentApp != AppID::MESSAGE && !appClock.isBusy() && !g_face.isSleeping()) {
            if (g_currentApp == AppID::GALLERY && appGallery.isPlayingVideo()) {
                // User is actively enjoying their custom video, don't interrupt!
                return;
            }
            AppID nextTab;
            switch (g_currentApp) {
                case AppID::FACE:       nextTab = AppID::CLOCK; break;
                case AppID::CLOCK:      nextTab = AppID::WEATHER; break;
                case AppID::WEATHER:    nextTab = AppID::WORLD_INFO; break;
                case AppID::WORLD_INFO:
                    nextTab = (appServer.isConfigured() && appServer.isOnline()) ? AppID::SERVER : (appGallery.hasPhoto() ? AppID::GALLERY : AppID::FACE);
                    break;
                case AppID::SERVER:
                    nextTab = appGallery.hasPhoto() ? AppID::GALLERY : AppID::FACE;
                    break;
                case AppID::GALLERY:    nextTab = AppID::FACE; break;
                default:                nextTab = AppID::CLOCK; break;
            }
            if (nextTab == AppID::GALLERY) {
                appGallery.setAutoCycle(true); // User requirement: Never play video in background cycle!
            }
            switchApp(nextTab);
        }
    }
    
    // 3. Frame rate cap
    if (!g_display.endFrame()) {
        delay(1); 
        return; 
    }
    
    // --- FRAME START ---
    g_display.beginFrame();
    
    // 4. Input processing
    TouchEvent ev = g_touchInput.update();
    if (ev != TouchEvent::NONE) {
        g_lastUserActivityTime = millis();
        
        // Absorb trailing finger-release taps or holds from menu selection
        if (millis() - g_lastAppSwitchTime < 550) {
            return;
        }
        
        // If Buddy is sleeping: any touch WAKES UP Buddy!
        if (g_face.isSleeping()) {
            g_face.wakeUp();
            lastAutoCycle = millis();
            return;
        }
        
        // Global TRIPLE TAP shortcut: Instant toggle between Pet Face and Digital Watch!
        if (ev == TouchEvent::TRIPLE_TAP) {
            if (g_menu.isActive()) {
                g_menu.hide();
                g_menu.clearSelectedApp();
            }
            if (g_currentApp == AppID::FACE) {
                appClock.setDigitalMode();
                switchApp(AppID::CLOCK);
            } else {
                switchApp(AppID::FACE);
            }
            lastAutoCycle = millis();
            return;
        }
        
        // Global DOUBLE TAP: Universal Back to Pet Face from ANY screen!
        if (ev == TouchEvent::DOUBLE_TAP) {
            if (g_menu.isActive()) {
                g_menu.hide();
                g_menu.clearSelectedApp();
                switchApp(AppID::FACE);
            } else if (g_currentApp == AppID::GAMES && appGames.isGameActive()) {
                // Inside an active arcade game (Dino/Flappy), pass touch to game so fast gameplay double-tap doesn't kill high score
                appGames.handleTouch(ev);
            } else if (g_currentApp != AppID::FACE) {
                // Universal back to Pet Face from Clock, Weather, Info, Settings, Gallery, Server, Games Menu!
                switchApp(AppID::FACE);
            } else {
                // Already on Pet Face: Double tap triggers instant affection / love reaction!
                g_face.onLongTouch();
            }
            lastAutoCycle = millis();
            return;
        }

        if (g_menu.isActive()) {
            g_menu.handleTouch(ev);
            if (g_menu.getSelectedApp() != AppID::NONE) {
                AppID nextApp = g_menu.getSelectedApp();
                g_menu.hide();
                g_menu.clearSelectedApp();
                switchApp(nextApp);
                lastAutoCycle = millis();
            }
        }
        else if (g_currentApp == AppID::FACE) {
            if (ev == TouchEvent::TAP) {
                // Single Tap: cycle pet expressions smoothly
                g_face.cycleNextExpression();
            } else if (ev == TouchEvent::LONG_PRESS || ev == TouchEvent::HOLD || ev == TouchEvent::DEEP_HOLD) {
                // Long Press (hold > 650ms): open Main Menu Carousel!
                g_display.captureScreen();
                g_menu.show();
            }
            lastAutoCycle = millis();
        }
        else if (g_currentApp == AppID::GAMES) {
            appGames.handleTouch(ev);
            lastAutoCycle = millis();
        }
        else if (g_currentApp == AppID::SETTINGS) {
            appSettings.handleTouch(ev);
            lastAutoCycle = millis();
        }
        else if (g_currentApp == AppID::GALLERY) {
            if (ev == TouchEvent::LONG_PRESS || ev == TouchEvent::HOLD || ev == TouchEvent::DEEP_HOLD) {
                // Long press opens Menu Carousel
                g_display.captureScreen();
                g_menu.show();
            } else {
                // Single tap toggles photo <-> video or invert; Double tap is handled globally to return to Pet Face
                appGallery.handleTouch(ev);
            }
            lastAutoCycle = millis();
        }
        else if (g_currentApp == AppID::MESSAGE) {
            if (ev == TouchEvent::LONG_PRESS || ev == TouchEvent::HOLD || ev == TouchEvent::DEEP_HOLD) {
                // Long press opens Menu Carousel
                g_display.captureScreen();
                g_menu.show();
            } else {
                // Single tap: next page of AI message; Double tap is handled globally to return to Pet Face
                appMessage.handleTouch(ev);
            }
            lastAutoCycle = millis();
        }
        else if (g_currentApp == AppID::SERVER) {
            if (!appServer.isOnline() || ev == TouchEvent::HOLD || ev == TouchEvent::DEEP_HOLD) {
                switchApp(AppID::FACE);
            } else if (ev == TouchEvent::LONG_PRESS) {
                g_display.captureScreen();
                g_menu.show();
            } else {
                appServer.handleTouch(ev);
            }
            lastAutoCycle = millis();
        }
        else {
            // Route all touches directly to Clock, Weather, World Info
            switch (g_currentApp) {
                case AppID::CLOCK: appClock.handleTouch(ev); break;
                case AppID::WEATHER: appWeather.handleTouch(ev); break;
                case AppID::WORLD_INFO: appWorldInfo.handleTouch(ev); break;
                default: break;
            }
            lastAutoCycle = millis();
        }
    }
    
    // 5. Update logic
    if (g_menu.isActive()) {
        g_menu.update();
        if (g_menu.getSelectedApp() != AppID::NONE) {
            AppID nextApp = g_menu.getSelectedApp();
            g_menu.hide();
            g_menu.clearSelectedApp();
            switchApp(nextApp);
            lastAutoCycle = millis();
        }
    } else {
        switch (g_currentApp) {
            case AppID::FACE: g_face.update(); break;
            case AppID::CLOCK: appClock.update(); break;
            case AppID::WEATHER: appWeather.update(); break;
            case AppID::WORLD_INFO: appWorldInfo.update(); break;
            case AppID::SERVER: appServer.update(); break;
            case AppID::GAMES: appGames.update(); break;
            case AppID::GALLERY: appGallery.update(); break;
            case AppID::SETTINGS: appSettings.update(); break;
            case AppID::MESSAGE: appMessage.update(); break;
            default: break;
        }
    }
    
    // 6. Render
    if (g_menu.isActive()) {
        g_menu.render();
    } else {
        switch (g_currentApp) {
            case AppID::FACE: g_face.render(); break;
            case AppID::CLOCK: appClock.render(); break;
            case AppID::WEATHER: appWeather.render(); break;
            case AppID::WORLD_INFO: appWorldInfo.render(); break;
            case AppID::SERVER: appServer.render(); break;
            case AppID::GAMES: appGames.render(); break;
            case AppID::GALLERY: appGallery.render(); break;
            case AppID::SETTINGS: appSettings.render(); break;
            case AppID::MESSAGE: appMessage.render(); break;
            default: break;
        }
    }
}
