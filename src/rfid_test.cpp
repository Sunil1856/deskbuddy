#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>

// ==========================================
// PIN CONFIGURATION (ESP32 Standard VSPI)
// ==========================================
#define SS_PIN    5   // SDA / SS (Chip Select)
#define RST_PIN  22   // RST (Reset)
#define SCK_PIN  18   // SCK (Clock)
#define MOSI_PIN 23   // MOSI (Data In to RC522)
#define MISO_PIN 19   // MISO (Data Out from RC522)

#define LED_PIN   2   // ESP32 Onboard Blue LED

MFRC522 mfrc522(SS_PIN, RST_PIN);

void setup() {
    Serial.begin(115200);
    delay(1000);

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    Serial.println("\n\n============================================");
    Serial.println("  ESP32 + RC522 High-Power Diagnostic Test ");
    Serial.println("============================================");

    SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);

    // Initialize MFRC522
    mfrc522.PCD_Init();
    delay(50);

    // ========================================================
    // BOOST ANTENNA GAIN TO MAXIMUM (48 dB)
    // Helps read power-hungry Metro cards (DESFire / NCMC) & phones
    // ========================================================
    mfrc522.PCD_SetAntennaGain(MFRC522::RxGain_max);
    Serial.println("[RC522] Antenna Gain set to MAXIMUM (48 dB)");

    // Read Reader Firmware Version
    byte version = mfrc522.PCD_ReadRegister(MFRC522::VersionReg);
    Serial.printf("[RC522] Hardware Version Check: 0x%02X\n", version);

    Serial.println("\nReady! Try scanning both your Keyfob and Metro Card...");
    Serial.println("Tip: For Metro cards, try holding it 5mm-10mm above reader or moving it across the edges.");
    Serial.println("----------------------------------------------------------------------------------------");
}

void loop() {
    static unsigned long lastPrompt = 0;

    // First try standard card check
    bool cardDetected = mfrc522.PICC_IsNewCardPresent();

    // If not detected with standard REQA, try WUPA (Wake-Up All for DESFire/Smart Cards)
    if (!cardDetected) {
        byte bufferATQA[2];
        byte bufferSize = sizeof(bufferATQA);
        mfrc522.PCD_WriteRegister(MFRC522::TxModeReg, 0x00);
        mfrc522.PCD_WriteRegister(MFRC522::RxModeReg, 0x00);
        mfrc522.PCD_WriteRegister(MFRC522::ModWidthReg, 0x26);
        MFRC522::StatusCode result = mfrc522.PICC_WakeupA(bufferATQA, &bufferSize);
        if (result == MFRC522::STATUS_OK) {
            cardDetected = true;
        }
    }

    if (!cardDetected) {
        if (millis() - lastPrompt > 4000) {
            Serial.println("[Waiting] Ready to scan card/tag (Max RF Power Active)...");
            lastPrompt = millis();
        }
        return;
    }

    // Read card serial / UID (handles both 4-byte and 7-byte UIDs)
    if (!mfrc522.PICC_ReadCardSerial()) {
        return;
    }

    // Turn ON blue LED for visual feedback
    digitalWrite(LED_PIN, HIGH);

    Serial.println("\n********************************************");
    Serial.println("              RFID TAG DETECTED!            ");
    Serial.println("********************************************");

    // Print UID in HEX format
    Serial.print(">> UID (HEX)  : ");
    for (byte i = 0; i < mfrc522.uid.size; i++) {
        if (mfrc522.uid.uidByte[i] < 0x10) Serial.print("0");
        Serial.print(mfrc522.uid.uidByte[i], HEX);
        if (i < mfrc522.uid.size - 1) Serial.print(":");
    }
    Serial.println();

    // Print UID in Decimal format
    Serial.print(">> UID (DEC)  : ");
    unsigned long long uidDec = 0;
    for (byte i = 0; i < mfrc522.uid.size; i++) {
        uidDec = (uidDec << 8) | mfrc522.uid.uidByte[i];
    }
    Serial.println((unsigned long)uidDec);

    // Print PICC Card Type (e.g. MIFARE Classic, MIFARE DESFire, etc.)
    MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
    Serial.print(">> Card Type  : ");
    Serial.println(mfrc522.PICC_GetTypeName(piccType));
    Serial.printf(">> UID Size   : %d bytes %s\n", mfrc522.uid.size, 
                  mfrc522.uid.size == 7 ? "(Standard Metro / DESFire size)" : "(Standard Keyfob size)");
    Serial.println("********************************************\n");

    delay(400);
    digitalWrite(LED_PIN, LOW);

    // Halt PICC
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();

    delay(800);
}
