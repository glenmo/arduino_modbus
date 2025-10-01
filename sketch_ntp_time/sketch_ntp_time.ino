/*
  UNO R4 WiFi + NTP (Australia/Melbourne)
  Requires:
    - WiFiS3 (by Arduino)
    - ezTime (by Rop Gonggrijp)
  Tools -> Board: "Arduino UNO R4 WiFi"

  Prints local time for Melbourne every second.
*/

#include <WiFiS3.h>
#include <ezTime.h>

const char* WIFI_SSID = "DER";
const char* WIFI_PASS = "GU23enY5!";

Timezone melbourne;   // ezTime timezone object
WiFiClient client;

void connectWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.print(WIFI_SSID);

  // Kick off connection
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  // Wait for connection (with a simple timeout)
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi connected. IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi connection timed out. Retrying in 3 seconds...");
    delay(3000);
  }
}

void setupTime() {
  // Use an AU pool; ezTime falls back as needed
  setServer("au.pool.ntp.org");

  Serial.print("Syncing time from NTP");
  waitForSync(20);  // wait up to 20s for NTP

  if (timeStatus() == timeSet) {
    Serial.println(" ... synced.");
  } else {
    Serial.println(" ... failed to sync (will keep trying in background).");
  }

  // Set timezone to Australia/Melbourne (DST handled automatically)
  melbourne.setLocation("Australia/Melbourne");
  Serial.print("Timezone set to: ");
  Serial.println(melbourne.getTimezoneName());
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  Serial.begin(115200);
  while (!Serial) { ; }

  // Bring up WiFi (retry until connected)
  while (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  // Initialize NTP/timezone
  setupTime();
}

void loop() {
  // Keep WiFi alive
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi dropped. Reconnecting...");
    connectWiFi();
  }

  // ezTime keeps time updated; this triggers housekeeping
  events();

  // Blink heartbeat
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));

  // Print local Melbourne time once per second
  static uint32_t lastPrint = 0;
  if (millis() - lastPrint >= 1000) {
    lastPrint = millis();
    // Example format: Tue 01 Oct 2025 17:05:03 AEST
    String ts = melbourne.dateTime("D d M Y H:i:s T");
    Serial.println(ts);
  }
}
