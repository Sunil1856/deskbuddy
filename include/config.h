#pragma once
// ============================================================================
// DeskBuddy — Hardware & Software Configuration
// ============================================================================

// --- Hardware Pins ---
#define PIN_SDA         21       // OLED I2C Data (ESP32 DevKit V1)
#define PIN_SCL         22       // OLED I2C Clock (ESP32 DevKit V1)
#define PIN_TOUCH       27       // TTP223 Capacitive Touch Sensor (digital)

// --- Display ---
#define SCREEN_W        128
#define SCREEN_H        64
#define OLED_ADDR       0x3C
#define OLED_RESET      -1      // No reset pin
#define TARGET_FPS      30
#define FRAME_TIME_MS   (1000 / TARGET_FPS)

// --- Touch Timing (ms) ---
#define TOUCH_DEBOUNCE_MS       28       // Balanced 28ms debounce (filters RF & electrical noise, retains snappy response)
#define TOUCH_TAP_MAX_MS        300      // Natural tap window (< 300ms)
#define TOUCH_DOUBLE_GAP_MS     350      // Relaxed 350ms gap so double-tap is effortless and reliable
#define TOUCH_LONG_PRESS_MS     650      // Crisp 650ms long press for Enter / Action / Menu
#define TOUCH_HOLD_MS           1400     // 1.4s hold threshold
#define TOUCH_DEEP_HOLD_MS      3000     // 3.0s deep hold
#define TOUCH_STUCK_TIMEOUT_MS  3500UL   // 3.5s watchdog to auto-reset stuck capacitive sensor

// --- WiFi AP (Captive Portal) ---
#define AP_SSID         "DeskBuddy-Setup"
#define AP_PASS         "buddy1234"
#define AP_CHANNEL      1

// --- NTP ---
#define NTP_SERVER_1    "pool.ntp.org"
#define NTP_SERVER_2    "time.nist.gov"
#define NTP_SYNC_INTERVAL_MS    3600000UL   // 1 hour

// --- HTTP User-Agent ---
// Modern web APIs (Google, CoinGecko, Open-Meteo, ZenQuotes) block default ESP32 headers
#define HTTP_USER_AGENT "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/122.0.0.0 Safari/537.36"

// --- APIs (Free & keyless by default, with optional API key support) ---
#define API_WEATHER             "https://api.open-meteo.com/v1/forecast"
#define API_GEOCODING           "https://geocoding-api.open-meteo.com/v1/search"
#define API_QUOTES              "https://zenquotes.io/api/random"
#define API_FACTS               "https://uselessfacts.jsph.pl/api/v2/facts/random?language=en"

// CoinGecko endpoint (with Binance keyless fallback if rate limited)
#define API_MARKETS_COINGECKO   "https://api.coingecko.com/api/v3/simple/price?ids=bitcoin,ethereum,pax-gold&vs_currencies=usd,inr"
#define API_MARKETS_BINANCE     "https://api.binance.com/api/v3/ticker/price?symbols=%5B%22BTCUSDT%22,%22ETHUSDT%22,%22PAXGUSDT%22%5D"

// Automatic IP Geolocation (Detects current city & coordinates on any Wi-Fi network)
#define API_IP_GEOLOCATION_1    "http://ip-api.com/json/?fields=status,city,lat,lon"
#define API_IP_GEOLOCATION_2    "http://ipwho.is/"

// Google News RSS feeds (parsed via lightweight streaming parser to eliminate OOM crashes)
#define API_NEWS_SPORTS         "https://news.google.com/rss/search?q=India+sport&hl=en-IN&gl=IN&ceid=IN:en"
#define API_NEWS_INDIA          "https://news.google.com/rss/search?q=India&hl=en-IN&gl=IN&ceid=IN:en"
#define API_NEWS_TECH           "https://news.google.com/rss/search?q=India+technology&hl=en-IN&gl=IN&ceid=IN:en"

// --- Optional API Keys ---
// If left empty (""), DeskBuddy uses 100% free keyless providers automatically!
// You can also configure them via the WiFi setup portal.
#ifndef OPENWEATHER_API_KEY
#define OPENWEATHER_API_KEY     ""      // Optional: openweathermap.org API key
#endif

#ifndef COINGECKO_API_KEY
#define COINGECKO_API_KEY       ""      // Optional: CoinGecko Demo API key
#endif

// --- API Refresh Intervals (ms) ---
#define WEATHER_REFRESH_MS      900000UL    // 15 min
#define INFO_REFRESH_MS         300000UL    // 5 min

// --- Face Animation ---
#define BLINK_MIN_INTERVAL_MS   2000
#define BLINK_MAX_INTERVAL_MS   6000
#define BLINK_CLOSE_MS          80
#define BLINK_OPEN_MS           150
#define EXPRESSION_TRANSITION_MS 300
#define IDLE_LOOK_INTERVAL_MS   3000
#define SLEEPY_TIMEOUT_MS       120000UL    // 2 min
#define SLEEP_TIMEOUT_MS        300000UL    // 5 min

#if __has_include("local_config.h")
#include "local_config.h"
#endif

// --- Pi Server Monitor (Local Network Companion) ---
#ifndef SERVER_MONITOR_URL
#define SERVER_MONITOR_URL      "http://192.168.x.x:34001/api/v1.0/get-history?n=1"
#endif

#ifndef SERVER_DISPLAY_NAME
#define SERVER_DISPLAY_NAME     "PI SERVER"
#endif

// --- Menu ---
#define MENU_ITEM_COUNT         8
#define MENU_ICON_SIZE          24
#define MENU_SCROLL_SPEED       2

// --- NVS Keys ---
#define NVS_NAMESPACE           "deskbuddy"
#define NVS_KEY_WIFI_SSID       "wifi_ssid"
#define NVS_KEY_WIFI_PASS       "wifi_pass"
#define NVS_KEY_TIMEZONE        "timezone"
#define NVS_KEY_LATITUDE        "latitude"
#define NVS_KEY_LONGITUDE       "longitude"
#define NVS_KEY_CITY_NAME       "city_name"
#define NVS_KEY_BRIGHTNESS      "brightness"
#define NVS_KEY_TIME_FMT        "time_fmt"
#define NVS_KEY_DISPLAY_FLIP    "disp_flip"
#define NVS_KEY_SLEEP_TIMEOUT   "sleep_to"
#define NVS_KEY_CLOCK_MODE      "clock_mode"
#define NVS_KEY_EYE_STYLE       "eye_style"
#define NVS_KEY_HAPPINESS       "happiness"
#define NVS_KEY_DINO_HISCORE    "dino_hi"
#define NVS_KEY_FLAPPY_HISCORE  "flappy_hi"
#define NVS_KEY_STACK_HISCORE   "stack_hi"
#define NVS_KEY_REACT_BEST      "react_best"
#define NVS_KEY_CONFIGURED      "configured"
#define NVS_KEY_OWM_KEY         "owm_key"
#define NVS_KEY_CG_KEY          "cg_key"
#define NVS_KEY_AUTOCYCLE       "autocycle"
#define NVS_KEY_PHOTO_DATA      "photo_data"
#define NVS_KEY_PHOTO_HAS       "photo_has"
#define NVS_KEY_PHOTO_INV       "photo_inv"

// --- Firmware Info ---
#define FW_NAME         "DeskBuddy"
#define FW_VERSION      "1.2.1"
#define FW_AUTHOR       "Open Source"
