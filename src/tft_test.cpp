#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

// ==========================================
// PIN CONFIGURATION (ESP32 Standard VSPI)
// ==========================================
#define TFT_CS        5   // Chip Select (CS)
#define TFT_DC        2   // Data/Command (DC / RS)
#define TFT_RST       4   // Reset (RST)
#define TFT_MOSI     23   // Master Out Slave In (MOSI / SDA / DIN)
#define TFT_SCLK     18   // SPI Clock (SCK / SCL / CLK)
#define TFT_MISO     -1   // Not connected for ST7789

// Note:
// 1. Backlight (BLK / LED) -> 3.3V (ya 5V agar module me 3.3V regulator ho)
// 2. VCC -> 3.3V (ya 5V agar module par LDO laga ho)
// 3. GND -> GND

// Hardware SPI constructor
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

void drawColorTest(const char* name, uint16_t color, uint16_t textColor) {
    tft.fillScreen(color);
    tft.setTextColor(textColor);
    tft.setTextSize(3);
    tft.setCursor(20, 140);
    tft.print(name);
    Serial.printf("[TFT] Screen Color: %s\n", name);
    delay(1000);
}

void drawShapesDemo() {
    tft.fillScreen(ST77XX_BLACK);
    
    // Header
    tft.fillRect(0, 0, 240, 30, ST77XX_BLUE);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(2);
    tft.setCursor(15, 7);
    tft.print("Graphics Test");

    // Rectangles
    tft.drawRect(20, 50, 60, 40, ST77XX_RED);
    tft.fillRect(100, 50, 60, 40, ST77XX_GREEN);
    tft.drawRoundRect(180, 50, 45, 40, 8, ST77XX_YELLOW);

    // Circles
    tft.drawCircle(50, 130, 25, ST77XX_CYAN);
    tft.fillCircle(130, 130, 25, ST77XX_MAGENTA);
    tft.drawCircle(200, 130, 25, ST77XX_WHITE);

    // Triangles
    tft.drawTriangle(50, 180, 20, 230, 80, 230, ST77XX_ORANGE);
    tft.fillTriangle(130, 180, 100, 230, 160, 230, ST77XX_GREEN);

    // Text info
    tft.setTextSize(1);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(20, 260);
    tft.print("Shapes & Primitives OK!");
    tft.setCursor(20, 280);
    tft.print("Resolution: 240 x 320");

    Serial.println("[TFT] Shapes demo completed");
    delay(2500);
}

void drawAnimationDemo() {
    tft.fillScreen(ST77XX_BLACK);
    tft.fillRect(0, 0, 240, 30, 0x51D7);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(2);
    tft.setCursor(25, 7);
    tft.print("Animation Test");

    int ballX = 120;
    int ballY = 160;
    int speedX = 4;
    int speedY = 5;
    const int radius = 12;

    unsigned long startTime = millis();
    while (millis() - startTime < 4000) {
        // Clear previous ball position
        tft.fillCircle(ballX, ballY, radius, ST77XX_BLACK);

        // Update position
        ballX += speedX;
        ballY += speedY;

        // Bounce from walls (keep below top bar)
        if (ballX - radius <= 0 || ballX + radius >= 240) speedX = -speedX;
        if (ballY - radius <= 32 || ballY + radius >= 310) speedY = -speedY;

        // Draw new ball position
        tft.fillCircle(ballX, ballY, radius, ST77XX_YELLOW);
        delay(20);
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n\n====================================");
    Serial.println("  ESP32 ST7789 2.4\" TFT Diagnostic  ");
    Serial.println("====================================");

    // Initialize SPI explicitly with chosen pins
    Serial.printf("[SPI] Initializing: SCK=%d, MOSI=%d, CS=%d\n", TFT_SCLK, TFT_MOSI, TFT_CS);
    SPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI, TFT_CS);

    // Initialize ST7789 display (240x320 resolution)
    Serial.println("[TFT] Initializing ST7789 display...");
    tft.init(240, 320);

    // Set rotation: 0=Portrait (240x320), 1=Landscape (320x240), 2=Inv Portrait, 3=Inv Landscape
    tft.setRotation(0);

    // NOTE: Agar screen par colors ulte dikhein (jaise RED ki jagah CYAN),
    // to neeche wali line ko uncomment karein:
    // tft.invertDisplay(true);

    Serial.println("[TFT] Display initialized successfully!");
    
    // Boot Splash
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(2);
    tft.setCursor(30, 80);
    tft.print("ESP32 TEST");
    tft.setTextColor(ST77XX_GREEN);
    tft.setCursor(20, 110);
    tft.print("ST7789 2.4\" TFT");
    tft.setTextColor(ST77XX_CYAN);
    tft.setTextSize(1);
    tft.setCursor(35, 150);
    tft.print("Starting Diagnostics...");
    tft.setCursor(35, 170);
    tft.print("Resolution: 240 x 320");
    delay(2000);
}

void loop() {
    static int loopCount = 1;
    Serial.printf("\n--- Starting Test Cycle #%d ---\n", loopCount++);

    // 1. Color Test
    drawColorTest("RED", ST77XX_RED, ST77XX_WHITE);
    drawColorTest("GREEN", ST77XX_GREEN, ST77XX_BLACK);
    drawColorTest("BLUE", ST77XX_BLUE, ST77XX_WHITE);
    drawColorTest("WHITE", ST77XX_WHITE, ST77XX_BLACK);
    drawColorTest("BLACK", ST77XX_BLACK, ST77XX_WHITE);

    // 2. Shapes & Primitives Test
    drawShapesDemo();

    // 3. Smooth Animation Test
    drawAnimationDemo();

    // 4. Status Card
    tft.fillScreen(ST77XX_BLACK);
    tft.drawRect(5, 5, 230, 310, ST77XX_GREEN);
    tft.drawRect(7, 7, 226, 306, ST77XX_GREEN);

    tft.setTextColor(ST77XX_YELLOW);
    tft.setTextSize(2);
    tft.setCursor(35, 30);
    tft.print("DISPLAY OK!");

    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(1);
    tft.setCursor(20, 80);
    tft.print("Driver: ST7789 (SPI)");
    tft.setCursor(20, 100);
    tft.printf("Uptime: %lu sec", millis() / 1000);
    tft.setCursor(20, 120);
    tft.printf("Cycles: %d", loopCount - 1);

    tft.setTextColor(ST77XX_CYAN);
    tft.setCursor(20, 160);
    tft.print("Pin Connections:");
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(20, 180);
    tft.print("MOSI/SDA -> GPIO 23");
    tft.setCursor(20, 200);
    tft.print("SCK/SCL  -> GPIO 18");
    tft.setCursor(20, 220);
    tft.print("CS       -> GPIO 5");
    tft.setCursor(20, 240);
    tft.print("DC/RS    -> GPIO 2");
    tft.setCursor(20, 260);
    tft.print("RST      -> GPIO 4");
    tft.setCursor(20, 280);
    tft.print("BLK/LED  -> 3.3V");

    Serial.println("[TFT] Cycle completed. Waiting 3 seconds before next cycle...");
    delay(3000);
}
