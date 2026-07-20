#include <Wire.h>
#include <WiFi.h>
#include <LiquidCrystal_I2C.h>

// ======================= CREDENTIALS CONFIG ====================
// TODO: Replace with your actual local Wi-Fi credentials
const char* ssid = "GSV_Electrical_Enterprises";
const char* password = "@26Nov1996#";

// Auto-detected LiquidCrystal_I2C pointer
LiquidCrystal_I2C* lcd = NULL;
uint8_t lcdAddr = 0;

/**
 * Scans the I2C bus for the display address.
 * Most standard I2C LCDs (16x2 or 20x4) use address 0x27 or 0x3F.
 */
uint8_t scanI2CBus(int &outSda, int &outScl) {
  struct I2CPinPair {
    int sda;
    int scl;
    const char* name;
  };

  // 1. First, check common I2C pin pairs with a full address scan (1 to 127)
  I2CPinPair commonPairs[] = {
    {21, 22, "Default (21, 22)"},
    {22, 21, "Swapped Default (22, 21)"},
    {33, 32, "Detected Wiring (33, 32)"},
    {32, 33, "Swapped Detected Wiring (32, 33)"},
    {25, 26, "Common A (25, 26)"},
    {26, 25, "Swapped Common A (26, 25)"}
  };

  Serial.println("\n[I2C] Phase 1: Scanning common pin configurations (full address range 1-127)...");
  int numCommon = sizeof(commonPairs) / sizeof(commonPairs[0]);
  
  for (int p = 0; p < numCommon; p++) {
    int sda = commonPairs[p].sda;
    int scl = commonPairs[p].scl;
    
    Serial.printf("[I2C] Scanning common pair: SDA=%d, SCL=%d (%s)...\n", sda, scl, commonPairs[p].name);
    Wire.end();
    Wire.begin(sda, scl);
    Wire.setClock(100000);
    Wire.setTimeOut(25);
    delay(10);
    
    for (uint8_t address = 1; address < 127; address++) {
      Wire.beginTransmission(address);
      if (Wire.endTransmission() == 0) {
        Serial.printf("[FOUND] I2C Device found at address: 0x%02X on SDA=%d, SCL=%d!\n", address, sda, scl);
        outSda = sda;
        outScl = scl;
        return address;
      }
    }
  }

  // 2. If Phase 1 finds nothing, run Phase 2: scan ALL bidirectional GPIO pins for common LCD addresses (0x27, 0x3F, 0x3E, 0x20)
  int bidirPins[] = {2, 4, 5, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33};
  int numPins = sizeof(bidirPins) / sizeof(bidirPins[0]);
  uint8_t lcdAddresses[] = {0x27, 0x3F, 0x3E, 0x20};
  int numLcdAddrs = sizeof(lcdAddresses) / sizeof(lcdAddresses[0]);

  Serial.println("\n[I2C] Phase 2: Scanning ALL bidirectional GPIO pin combinations for LCD addresses (0x27, 0x3F)...");
  
  for (int i = 0; i < numPins; i++) {
    for (int j = 0; j < numPins; j++) {
      if (i == j) continue;
      int sda = bidirPins[i];
      int scl = bidirPins[j];
      
      Wire.end();
      Wire.begin(sda, scl);
      Wire.setClock(100000);
      Wire.setTimeOut(10); // Short timeout for speed
      
      for (int a = 0; a < numLcdAddrs; a++) {
        Wire.beginTransmission(lcdAddresses[a]);
        if (Wire.endTransmission() == 0) {
          Serial.printf("[FOUND] LCD detected at address: 0x%02X on Custom Pins: SDA=%d, SCL=%d!\n", lcdAddresses[a], sda, scl);
          outSda = sda;
          outScl = scl;
          return lcdAddresses[a];
        }
      }
    }
  }

  Serial.println("[Error] No devices responded during either phase of the bus scan.");
  return 0;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=================================");
  Serial.println("  ESP32 WiFi LCD Display Tester  ");
  Serial.println("=================================");

  // 1. Scan for I2C LCD Display
  int sdaPin = 21;
  int sclPin = 22;
  lcdAddr = scanI2CBus(sdaPin, sclPin);
  if (lcdAddr == 0) {
    Serial.println("[Error] No I2C LCD Display found!");
    Serial.println("[Hint] Check hardware pins: SDA -> GPIO 21, SCL -> GPIO 22");
    Serial.println("[Hint] Make sure the LCD's I2C backpack has VCC connected to 5V and GND to GND.");
  } else {
    Serial.printf("[LCD] Auto-detected LCD at address: 0x%02X on SDA=%d, SCL=%d\n", lcdAddr, sdaPin, sclPin);
    
    // Initialize Wire on the correct pins
    Wire.end();
    Wire.begin(sdaPin, sclPin);
    
    // Initialize the auto-detected LCD
    lcd = new LiquidCrystal_I2C(lcdAddr, 16, 2);
    lcd->init();
    
    // Force Wire back to correct pins since lcd->init() overrides pins to default 21, 22
    Wire.begin(sdaPin, sclPin);
    
    lcd->backlight();
    
    // Print initial connecting status
    lcd->setCursor(0, 0);
    lcd->print("WiFi: Connecting");
    lcd->setCursor(0, 1);
    lcd->print("SSID: ");
    lcd->print(ssid);
  }

  // 2. Connect to WiFi
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  
  Serial.printf("[WiFi] Connecting to %s...\n", ssid);
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    
    // Animate a loading bar or dots on LCD
    if (lcd) {
      lcd->setCursor(attempts % 16, 0);
      // Toggle a character to show activity
      lcd->print(attempts % 2 == 0 ? ">" : " ");
    }
    attempts++;
  }

  // 3. Show Connection Result
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WiFi] Connected successfully!");
    Serial.print("[WiFi] IP Address: ");
    Serial.println(WiFi.localIP());

    if (lcd) {
      lcd->clear();
      lcd->setCursor(0, 0);
      lcd->print("WiFi Connected!");
      lcd->setCursor(0, 1);
      lcd->print(WiFi.localIP().toString().c_str());
    }
  } else {
    Serial.println("\n[WiFi] Connection timed out / failed.");
    
    if (lcd) {
      lcd->clear();
      lcd->setCursor(0, 0);
      lcd->print("Conn Failed!");
      lcd->setCursor(0, 1);
      lcd->print("Check SSID/Pass");
    }
  }
}

void loop() {
  // Standby loop
  if (WiFi.status() != WL_CONNECTED) {
    // If WiFi drops, show warning
    if (lcd) {
      lcd->setCursor(0, 0);
      lcd->print("WiFi Connection");
      lcd->setCursor(0, 1);
      lcd->print("Lost! Retrying..");
    }
    WiFi.disconnect();
    WiFi.begin(ssid, password);
    delay(5000);
  }
  delay(1000);
}
