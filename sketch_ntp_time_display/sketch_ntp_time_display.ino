/*
  UNO R4 WiFi - Built-in 12x8 LED Matrix - NTP Melbourne Time Scroller
  - Wi-Fi SSID: "DER", key: "GU23enY5!"
  - Uses ezTime for timezone & DST (Australia/Melbourne)
  - Scrolls "HH:MM" across the 12x8 matrix

  Libraries:
    - WiFiS3 (Arduino)
    - ezTime (Rop Gonggrijp)
    - Arduino_LED_Matrix (Arduino)
*/

#include <WiFiS3.h>
#include <ezTime.h>
#include <Arduino_LED_Matrix.h>

const char* WIFI_SSID = "DER";
const char* WIFI_PASS = "GU23enY5!";

ArduinoLEDMatrix matrix;
Timezone tzMel;

// ---- Minimal 3x5 digit font (columns; only lower 5 bits used) ----
struct Glyph { uint8_t col[3]; };

const Glyph DIGITS[10] = {
  /*0*/ {{0b01110, 0b10001, 0b11111}},
  /*1*/ {{0b00000, 0b00000, 0b11111}},
  /*2*/ {{0b11001, 0b10101, 0b10011}},
  /*3*/ {{0b10001, 0b10101, 0b11111}},
  /*4*/ {{0b00111, 0b00100, 0b11111}},
  /*5*/ {{0b10111, 0b10101, 0b11101}},
  /*6*/ {{0b11111, 0b10101, 0b11101}},
  /*7*/ {{0b00001, 0b00001, 0b11111}},
  /*8*/ {{0b11111, 0b10101, 0b11111}},
  /*9*/ {{0b10111, 0b10101, 0b11111}},
};

const uint8_t COLON_COL = 0b00101;  // two dots in 5 rows

// Our working frame as booleans; later we pack to uint32_t[3]
bool frame[8][12];

// --- Helpers to manipulate the boolean frame ---
void clearFrame() {
  for (int r = 0; r < 8; r++)
    for (int c = 0; c < 12; c++)
      frame[r][c] = false;
}

inline void setPixel(int r, int c, bool on) {
  if (r >= 0 && r < 8 && c >= 0 && c < 12) frame[r][c] = on;
}

// Pack boolean frame[8][12] into 3×uint32_t (96 bits) and load
void pushFrameToMatrix() {
  uint32_t packed[3] = {0, 0, 0};
  // Bit index grows left-to-right, top-to-bottom: (row * 12 + col)
  for (int r = 0; r < 8; r++) {
    for (int c = 0; c < 12; c++) {
      if (frame[r][c]) {
        int bitIndex = r * 12 + c;           // 0..95
        int word = bitIndex / 32;            // 0..2
        int bit  = bitIndex % 32;            // 0..31
        packed[word] |= (1UL << bit);
      }
    }
  }
  matrix.loadFrame(packed);  // <-- correct signature: const uint32_t[3]
}

// Draw a single 8-bit column into a matrix column (vertically centered)
void blitColumn(uint8_t packedCol, int targetCol) {
  const int yOffset = 1; // center 5 rows within 8
  for (int row = 0; row < 5; row++) {
    bool on = (packedCol >> row) & 0x01;
    setPixel(yOffset + row, targetCol, on);
  }
}

// Build columns for a string like "12:34"
int buildTimeColumns(const char* s, uint8_t outCols[], int maxCols) {
  int n = 0;
  for (const char* p = s; *p; ++p) {
    if (*p >= '0' && *p <= '9') {
      int d = *p - '0';
      for (int i = 0; i < 3 && n < maxCols; i++) outCols[n++] = DIGITS[d].col[i];
      if (n < maxCols) outCols[n++] = 0x00; // spacer
    } else if (*p == ':') {
      if (n < maxCols) outCols[n++] = COLON_COL;
      if (n < maxCols) outCols[n++] = 0x00; // spacer
    } else {
      if (n < maxCols) outCols[n++] = 0x00;
    }
  }
  return n;
}

// Render a 12-col window from a longer column buffer at 'offset'
void renderWindow(const uint8_t srcCols[], int srcLen, int offset) {
  clearFrame();
  for (int c = 0; c < 12; c++) {
    int idx = offset + c;
    uint8_t colBits = (idx >= 0 && idx < srcLen) ? srcCols[idx] : 0x00;
    blitColumn(colBits, c);
  }
  pushFrameToMatrix();
}

// Wi-Fi + time setup
void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
    delay(250);
  }
}

void setupTime() {
  setServer("au.pool.ntp.org");
  waitForSync(20);
  tzMel.setLocation("Australia/Melbourne");
}

// Format local time as "HH:MM"
String getTimeString() {
  return tzMel.dateTime("H:i");
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {}

  matrix.begin();

  connectWiFi();
  setupTime();

  // Simple alive mark
  clearFrame();
  setPixel(3, 5, true); setPixel(4, 6, true);
  pushFrameToMatrix();
  delay(400);
}

void loop() {
  // Wi-Fi keepalive (light touch)
  if (WiFi.status() != WL_CONNECTED) {
    static unsigned long lastTry = 0;
    if (millis() - lastTry > 5000) {
      lastTry = millis();
      WiFi.begin(WIFI_SSID, WIFI_PASS);
    }
  }

  // Update time text when minute changes
  static String lastTime = "";
  String nowStr = getTimeString();
  if (nowStr != lastTime) lastTime = nowStr;

  // Build columns for current time and scroll them
  static uint8_t cols[64];
  int len = buildTimeColumns(lastTime.c_str(), cols, sizeof(cols));

  for (int offset = -12; offset <= len; offset++) {
    renderWindow(cols, len, offset);
    delay(80);     // scroll speed
    events();      // ezTime housekeeping

    // If minute rolled over during the scroll, refresh immediately
    String check = getTimeString();
    if (check != lastTime) {
      lastTime = check;
      break;
    }
  }
}
