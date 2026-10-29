#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <esp_heap_caps.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define PIN_SDA 21
#define PIN_SCL 22
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

bool oledFound = false;
uint8_t oledAddress = 0x3C;
bool heartbeat = false;

void updateDisplay() {
    if (!oledFound) return;

    // Physical Hardware Specs
    uint32_t flashSize     = ESP.getFlashChipSize();
    uint32_t internalTotal = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
    bool hasPsram          = psramFound();
    uint32_t psramTotal    = 0;
    if (hasPsram) {
        psramTotal = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    }

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);

    // --- Header ---
    display.setCursor(2, 0);
    display.print("== CHIP SPECS (HW) ==");
    
    // Heartbeat indicator in top right
    if (heartbeat) {
        display.fillCircle(124, 3, 2, SSD1306_WHITE);
    } else {
        display.drawCircle(124, 3, 2, SSD1306_WHITE);
    }

    display.drawLine(0, 9, 127, 9, SSD1306_WHITE);

    // 1. Chip Model & Revision
    display.setCursor(0, 11);
    display.printf("Chip : %s (r%d)", ESP.getChipModel(), ESP.getChipRevision());

    // 2. CPU Specs
    display.setCursor(0, 20);
    display.printf("CPU  : 2-Core @ %luMHz", (unsigned long)ESP.getCpuFreqMHz());

    // 3. Physical Flash Memory
    display.setCursor(0, 29);
    display.printf("Flash: %lu MB (Physical)", (unsigned long)(flashSize / (1024 * 1024)));

    // 4. Hardware SRAM
    display.setCursor(0, 38);
    display.print("SRAM : 520 KB (Hardware)");

    // 5. Usable Internal Heap RAM
    display.setCursor(0, 47);
    display.printf("Heap : %lu KB (Internal)", (unsigned long)(internalTotal / 1024));

    // 6. External PSRAM
    display.setCursor(0, 56);
    if (hasPsram && psramTotal > 0) {
        display.printf("PSRAM: %.1f MB (External)", (float)psramTotal / (1024.0 * 1024.0));
    } else {
        display.print("PSRAM: 0 MB (Not Present)");
    }

    display.display();
}

void printSerialReport() {
    uint32_t flashSize     = ESP.getFlashChipSize();
    uint32_t internalTotal = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
    bool hasPsram          = psramFound();
    uint32_t psramTotal    = 0;
    if (hasPsram) {
        psramTotal = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    }

    Serial.println("\n==========================================");
    Serial.println("       ESP32 RAW HARDWARE SPEC SHEET      ");
    Serial.println("==========================================");
    Serial.printf("1. CHIP MODEL    : %s\n", ESP.getChipModel());
    Serial.printf("2. SILICON REV   : %d\n", ESP.getChipRevision());
    Serial.printf("3. CPU CORES     : 2 Cores (Xtensa LX6)\n");
    Serial.printf("4. CPU CLOCK     : %lu MHz\n", (unsigned long)ESP.getCpuFreqMHz());
    Serial.printf("5. FLASH MEMORY  : %lu MB (Hardware SPI Flash Chip)\n", 
                  (unsigned long)(flashSize / (1024 * 1024)));
    Serial.printf("6. ON-CHIP SRAM  : 520 KB (Hardware SRAM)\n");
    Serial.printf("7. USABLE HEAP   : %lu KB (%lu bytes)\n", 
                  (unsigned long)(internalTotal / 1024), (unsigned long)internalTotal);
    if (hasPsram && psramTotal > 0) {
        Serial.printf("8. EXTERNAL PSRAM: %.2f MB (%lu KB)\n", 
                      (float)psramTotal / (1024.0 * 1024.0), (unsigned long)(psramTotal / 1024));
    } else {
        Serial.printf("8. EXTERNAL PSRAM: 0 MB (No external PSRAM chip on module)\n");
    }
    Serial.println("==========================================\n");
}

void setup() {
    Serial.begin(115200);
    delay(500);

    Wire.begin(PIN_SDA, PIN_SCL);
    Wire.setClock(400000);

    if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        oledFound = true;
        oledAddress = 0x3C;
    } else if (display.begin(SSD1306_SWITCHCAPVCC, 0x3D)) {
        oledFound = true;
        oledAddress = 0x3D;
    }

    if (oledFound) {
        display.clearDisplay();
        display.display();
    }

    printSerialReport();
    updateDisplay();
}

void loop() {
    heartbeat = !heartbeat;
    updateDisplay();
    delay(1000);
}
