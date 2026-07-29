/**
 * cat_fun_print_test.ino
 * ──────────────────────────────────────────────────────────────────
 * Minimal stand-alone test sketch: ESP32 → CAT Fun BLE Printer
 *
 * WHAT IT DOES
 *   1. Initialises the ESP32 BLE stack
 *   2. Connects to the CAT Fun printer by the MAC address below
 *   3. Sends a sample ticket as 384-pixel-wide bitmap rows
 *   4. Disconnects and enters deep sleep
 *
 * HOW TO USE
 *   - Set CAT_PRINTER_MAC to the MAC of your printer
 *     (find it on the label or via a BLE scanner app)
 *   - Flash to ESP32 (COM8)
 *   - Open Serial Monitor at 115200 baud to see progress
 * ──────────────────────────────────────────────────────────────────
 */

#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEClient.h>
#include <BLEUtils.h>
#include <BLEAdvertisedDevice.h>

// ──────────────────────────────────────────────────────────────────
//  ★  CHANGE THIS TO YOUR PRINTER'S BLE MAC ADDRESS  ★
//  (Check the sticker on the printer, or use a BLE scanner app)
//  Example: "67:BF:94:05:F9:7B"
// ──────────────────────────────────────────────────────────────────
const char* CAT_PRINTER_MAC = "67:BF:94:05:F9:7B";

// CAT Fun printer BLE UUIDs (GT01 / GB01 / MX06 series)
static BLEUUID CAT_SERVICE_UUID("0000ae30-0000-1000-8000-00805f9b34fb");
static BLEUUID CAT_WRITE_UUID  ("0000ae01-0000-1000-8000-00805f9b34fb");

BLEClient*               catClient    = nullptr;
BLERemoteCharacteristic* catWriteChar = nullptr;

// 48-byte canvas = one 384-pixel-wide paper row
static uint8_t canvas[48];

// ──────────────────────────────────────────────────────────────────
//  5×8 bitmap font  (ASCII 32–127, stored in PROGMEM)
//  Each entry: 5 column-bytes.  Column-byte bit 0 = TOP pixel row.
// ──────────────────────────────────────────────────────────────────
static const uint8_t PROGMEM font5x8[][5] = {
  {0x00,0x00,0x00,0x00,0x00}, // 32 ' '
  {0x00,0x00,0x5F,0x00,0x00}, // 33 '!'
  {0x00,0x07,0x00,0x07,0x00}, // 34 '"'
  {0x14,0x7F,0x14,0x7F,0x14}, // 35 '#'
  {0x24,0x2A,0x7F,0x2A,0x12}, // 36 '$'
  {0x23,0x13,0x08,0x64,0x62}, // 37 '%'
  {0x36,0x49,0x55,0x22,0x50}, // 38 '&'
  {0x00,0x05,0x03,0x00,0x00}, // 39 '''
  {0x00,0x1C,0x22,0x41,0x00}, // 40 '('
  {0x00,0x41,0x22,0x1C,0x00}, // 41 ')'
  {0x14,0x08,0x3E,0x08,0x14}, // 42 '*'
  {0x08,0x08,0x3E,0x08,0x08}, // 43 '+'
  {0x00,0x50,0x30,0x00,0x00}, // 44 ','
  {0x08,0x08,0x08,0x08,0x08}, // 45 '-'
  {0x00,0x60,0x60,0x00,0x00}, // 46 '.'
  {0x20,0x10,0x08,0x04,0x02}, // 47 '/'
  {0x3E,0x51,0x49,0x45,0x3E}, // 48 '0'
  {0x00,0x42,0x7F,0x40,0x00}, // 49 '1'
  {0x42,0x61,0x51,0x49,0x46}, // 50 '2'
  {0x21,0x41,0x45,0x4B,0x31}, // 51 '3'
  {0x18,0x14,0x12,0x7F,0x10}, // 52 '4'
  {0x27,0x45,0x45,0x45,0x39}, // 53 '5'
  {0x3C,0x4A,0x49,0x49,0x30}, // 54 '6'
  {0x01,0x71,0x09,0x05,0x03}, // 55 '7'
  {0x36,0x49,0x49,0x49,0x36}, // 56 '8'
  {0x06,0x49,0x49,0x29,0x1E}, // 57 '9'
  {0x00,0x36,0x36,0x00,0x00}, // 58 ':'
  {0x00,0x56,0x36,0x00,0x00}, // 59 ';'
  {0x08,0x14,0x22,0x41,0x00}, // 60 '<'
  {0x14,0x14,0x14,0x14,0x14}, // 61 '='
  {0x00,0x41,0x22,0x14,0x08}, // 62 '>'
  {0x02,0x01,0x51,0x09,0x06}, // 63 '?'
  {0x32,0x49,0x79,0x41,0x3E}, // 64 '@'
  {0x7E,0x11,0x11,0x11,0x7E}, // 65 'A'
  {0x7F,0x49,0x49,0x49,0x36}, // 66 'B'
  {0x3E,0x41,0x41,0x41,0x22}, // 67 'C'
  {0x7F,0x41,0x41,0x22,0x1C}, // 68 'D'
  {0x7F,0x49,0x49,0x49,0x41}, // 69 'E'
  {0x7F,0x09,0x09,0x09,0x01}, // 70 'F'
  {0x3E,0x41,0x49,0x49,0x7A}, // 71 'G'
  {0x7F,0x08,0x08,0x08,0x7F}, // 72 'H'
  {0x00,0x41,0x7F,0x41,0x00}, // 73 'I'
  {0x20,0x40,0x41,0x3F,0x01}, // 74 'J'
  {0x7F,0x08,0x14,0x22,0x41}, // 75 'K'
  {0x7F,0x40,0x40,0x40,0x40}, // 76 'L'
  {0x7F,0x02,0x0C,0x02,0x7F}, // 77 'M'
  {0x7F,0x04,0x08,0x10,0x7F}, // 78 'N'
  {0x3E,0x41,0x41,0x41,0x3E}, // 79 'O'
  {0x7F,0x09,0x09,0x09,0x06}, // 80 'P'
  {0x3E,0x41,0x51,0x21,0x5E}, // 81 'Q'
  {0x7F,0x09,0x19,0x29,0x46}, // 82 'R'
  {0x46,0x49,0x49,0x49,0x31}, // 83 'S'
  {0x01,0x01,0x7F,0x01,0x01}, // 84 'T'
  {0x3F,0x40,0x40,0x40,0x3F}, // 85 'U'
  {0x1F,0x20,0x40,0x20,0x1F}, // 86 'V'
  {0x3F,0x40,0x38,0x40,0x3F}, // 87 'W'
  {0x63,0x14,0x08,0x14,0x63}, // 88 'X'
  {0x07,0x08,0x70,0x08,0x07}, // 89 'Y'
  {0x61,0x51,0x49,0x45,0x43}, // 90 'Z'
  {0x00,0x7F,0x41,0x41,0x00}, // 91 '['
  {0x02,0x04,0x08,0x10,0x20}, // 92 '\'
  {0x00,0x41,0x41,0x7F,0x00}, // 93 ']'
  {0x04,0x02,0x01,0x02,0x04}, // 94 '^'
  {0x40,0x40,0x40,0x40,0x40}, // 95 '_'
  {0x00,0x01,0x02,0x04,0x00}, // 96 '`'
  {0x20,0x54,0x54,0x54,0x78}, // 97 'a'
  {0x7F,0x48,0x44,0x44,0x38}, // 98 'b'
  {0x38,0x44,0x44,0x44,0x20}, // 99 'c'
  {0x38,0x44,0x44,0x48,0x7F}, // 100 'd'
  {0x38,0x54,0x54,0x54,0x18}, // 101 'e'
  {0x08,0x7E,0x09,0x01,0x02}, // 102 'f'
  {0x0C,0x52,0x52,0x52,0x3E}, // 103 'g'
  {0x7F,0x08,0x04,0x04,0x78}, // 104 'h'
  {0x00,0x44,0x7D,0x40,0x00}, // 105 'i'
  {0x20,0x40,0x44,0x3D,0x00}, // 106 'j'
  {0x7F,0x10,0x28,0x44,0x00}, // 107 'k'
  {0x00,0x41,0x7F,0x40,0x00}, // 108 'l'
  {0x7C,0x04,0x18,0x04,0x78}, // 109 'm'
  {0x7C,0x08,0x04,0x04,0x78}, // 110 'n'
  {0x38,0x44,0x44,0x44,0x38}, // 111 'o'
  {0x7C,0x14,0x14,0x14,0x08}, // 112 'p'
  {0x08,0x14,0x14,0x18,0x7C}, // 113 'q'
  {0x7C,0x08,0x04,0x04,0x08}, // 114 'r'
  {0x48,0x54,0x54,0x54,0x20}, // 115 's'
  {0x04,0x3F,0x44,0x40,0x20}, // 116 't'
  {0x3C,0x40,0x40,0x20,0x7C}, // 117 'u'
  {0x1C,0x20,0x40,0x20,0x1C}, // 118 'v'
  {0x3C,0x40,0x30,0x40,0x3C}, // 119 'w'
  {0x44,0x28,0x10,0x28,0x44}, // 120 'x'
  {0x0C,0x50,0x50,0x50,0x3C}, // 121 'y'
  {0x44,0x64,0x54,0x4C,0x44}, // 122 'z'
  {0x00,0x08,0x36,0x41,0x00}, // 123 '{'
  {0x00,0x00,0x7F,0x00,0x00}, // 124 '|'
  {0x00,0x41,0x36,0x08,0x00}, // 125 '}'
  {0x10,0x08,0x08,0x10,0x08}, // 126 '~'
  {0x00,0x00,0x00,0x00,0x00}  // 127
};

// ──────────────────────────────────────────────────────────────────
//  CAT Fun Protocol helpers
// ──────────────────────────────────────────────────────────────────

uint8_t crc8cat(const uint8_t* data, size_t len) {
  uint8_t crc = 0;
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (int j = 0; j < 8; j++)
      crc = (crc & 0x80) ? (crc << 1) ^ 0x07 : (crc << 1);
  }
  return crc;
}

// Build + send one CAT Fun packet: [0x51][0x78][CMD][0x00][LENL][LENH][data][CRC][0xFF]
void catSendCmd(uint8_t cmd, const uint8_t* data, uint16_t dataLen) {
  if (!catWriteChar) return;
  if (dataLen > 48) dataLen = 48;
  static uint8_t pkt[56];
  pkt[0]=0x51; pkt[1]=0x78; pkt[2]=cmd; pkt[3]=0x00;
  pkt[4]=(uint8_t)(dataLen&0xFF); pkt[5]=(uint8_t)((dataLen>>8)&0xFF);
  if (data && dataLen>0) memcpy(pkt+6, data, dataLen);
  pkt[6+dataLen] = (data && dataLen>0) ? crc8cat(data,dataLen) : 0;
  pkt[7+dataLen] = 0xFF;
  catWriteChar->writeValue(pkt, dataLen+8, false);
  delay(20);
}

// ──────────────────────────────────────────────────────────────────
//  Canvas / bitmap row helpers
// ──────────────────────────────────────────────────────────────────
void canvasClear()      { memset(canvas,0,48); }
void canvasSetPixel(int x) { if(x>=0&&x<384) canvas[x>>3]|=(0x80>>(x&7)); }
void catSendRow()       { catSendCmd(0xA9, canvas, 48); }

// Print a text line at scale, centred or left-aligned
void catPrintText(const char* text, int scale, bool center) {
  int len=strlen(text);
  int lineW=len*6*scale;
  int startX=center?max(0,(384-lineW)/2):4;
  for (int pr=0; pr<8*scale; pr++) {
    canvasClear();
    int charRow=pr/scale;
    int x=startX;
    for (int i=0;i<len;i++) {
      uint8_t c=(uint8_t)text[i];
      if(c<32||c>127) c=63;
      for(int col=0;col<5;col++){
        uint8_t cd=pgm_read_byte(&font5x8[c-32][col]);
        if((cd>>charRow)&1){
          for(int sx=0;sx<scale;sx++) canvasSetPixel(x+col*scale+sx);
        }
      }
      x+=6*scale;
    }
    catSendRow();
    delay(4);
  }
}

void catPrintSeparator(int thickness=2){
  for(int i=0;i<thickness;i++){memset(canvas,0xFF,48);catSendRow();delay(4);}
}
void catPrintBlank(int rows=4){
  canvasClear();
  for(int i=0;i<rows;i++){catSendRow();delay(4);}
}

// ──────────────────────────────────────────────────────────────────
//  BLE connect to CAT Fun printer
// ──────────────────────────────────────────────────────────────────
bool catConnect(const char* mac) {
  Serial.printf("[CAT] Connecting to %s ...\n", mac);
  catClient = BLEDevice::createClient();
  if (!catClient->connect(BLEAddress(mac))) {
    Serial.println("[CAT] BLE connect FAILED");
    delete catClient; catClient=nullptr;
    return false;
  }
  Serial.println("[CAT] BLE GATT connected. Discovering services...");

  BLERemoteService* svc = catClient->getService(CAT_SERVICE_UUID);
  if (!svc) {
    // Try alternate UUID used by some CAT Fun hardware revisions
    svc = catClient->getService(BLEUUID("49535343-fe7d-4ae5-8fa9-9fafd205e455"));
  }
  if (!svc) { Serial.println("[CAT] Service NOT found!"); catClient->disconnect(); delete catClient; catClient=nullptr; return false; }

  catWriteChar = svc->getCharacteristic(CAT_WRITE_UUID);
  if (!catWriteChar) {
    catWriteChar = svc->getCharacteristic(BLEUUID("49535343-8841-43f4-a8d4-ecbe34729bb3"));
  }
  if (!catWriteChar) { Serial.println("[CAT] Write char NOT found!"); catClient->disconnect(); delete catClient; catClient=nullptr; return false; }

  Serial.println("[CAT] Printer ready!");
  return true;
}

// ──────────────────────────────────────────────────────────────────
//  Print the sample ticket
// ──────────────────────────────────────────────────────────────────
void printSampleTicket() {
  Serial.println("[PRINT] Starting sample ticket...");

  // ① Set darkness: cmd 0xAF, data [0x35]
  uint8_t energy[]={0x35};
  catSendCmd(0xAF, energy, 1);
  delay(80);

  // ② Print-start: cmd 0xA6, no data
  catSendCmd(0xA6, nullptr, 0);
  delay(80);

  // ③ Bitmap ticket content
  catPrintBlank(4);
  catPrintSeparator(3);
  catPrintBlank(3);
  catPrintText("Smart Clinic",    2, true);   // org name — scale 2 centred
  catPrintText("WALK-IN TICKET",  1, true);
  catPrintBlank(2);
  catPrintSeparator(2);
  catPrintBlank(4);
  catPrintText("Your Token Number:", 1, true);
  catPrintBlank(3);
  catPrintText("7",               5, true);   // token number — scale 5 (huge)
  catPrintBlank(3);
  catPrintText("General Service", 2, true);
  catPrintBlank(2);
  catPrintSeparator(2);
  catPrintBlank(2);
  catPrintText("Customer: Walk-In",     1, false);
  catPrintText("Est. Wait: 10 mins",    1, false);
  catPrintText("Printed: 22/07/2026",   1, false);
  catPrintBlank(2);
  catPrintSeparator(2);
  catPrintBlank(20);   // paper feed gap

  Serial.println("[PRINT] Sample ticket complete!");
}

// ──────────────────────────────────────────────────────────────────
//  Arduino setup() — runs once
// ──────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n====================================");
  Serial.println("  CAT Fun BLE Print Test - ESP32");
  Serial.println("====================================");
  Serial.printf("Target printer MAC: %s\n\n", CAT_PRINTER_MAC);

  // Init BLE stack
  BLEDevice::init("ESP32-CatPrintTest");
  BLEDevice::setPower(ESP_PWR_LVL_P9);
  Serial.println("[BLE] Stack initialized.");

  // Connect to printer
  if (!catConnect(CAT_PRINTER_MAC)) {
    Serial.println("\n[ERROR] Could not connect to printer.");
    Serial.println("  → Make sure the CAT Fun printer is powered ON");
    Serial.println("  → Make sure the MAC address is correct");
    Serial.println("  → Make sure it is not connected to a phone/app");
    Serial.println("\nRetrying in 5 seconds...");
    delay(5000);
    if (!catConnect(CAT_PRINTER_MAC)) {
      Serial.println("[ERROR] Second attempt also failed. Halting.");
      return;
    }
  }

  // Print the sample ticket
  printSampleTicket();

  // Disconnect
  if (catClient && catClient->isConnected()) catClient->disconnect();
  Serial.println("[BLE] Disconnected. Done!");
  Serial.println("\n>>> If paper came out: SUCCESS! <<<");
  Serial.println(">>> If nothing printed: check MAC address <<<");
}

void loop() {
  // Nothing — test is one-shot in setup()
  delay(1000);
}
