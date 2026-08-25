#include "config.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>

// ---------- Wi-Fi + MQTT Config ----------
const char* ssid       = WIFI_SSID;     // 🔹 primary Wi-Fi
const char* password   = WIFI_PASSWORD;       // 🔹 primary Wi-Fi password

// 🔹 fallback Wi-Fi 
const char* ssid2      = WIFI_SSID_FALLBACK;
const char* password2  = WIFI_PASSWORD_FALLBACK;

// 🔹 Google Cloud VM External IP (Mosquitto broker)
const char* mqtt_server = MQTT_SERVER;

// Device identity
#define DEVICE_ID "internal1c"

// ---------- MQTT ----------
WiFiClient espClient;
PubSubClient client(espClient);

// ---------- Sensor pins ----------
#define MQ9_AO   32
#define MQ135_AO 33
#define SDA_PIN  21
#define SCL_PIN  22
#define SEALEVELPRESSURE_HPA (1013.25)

// ---------- Sensor objects ----------
Adafruit_BMP280 bmp;
bool useBMP = false;

// ---------- Wi-Fi Setup ----------
void setup_wifi() {
  delay(10);
  Serial.println();

  // Try primary first
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 12000) {
    delay(500);
    Serial.print(".");
  }

  // If primary fails, try fallback
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\n❌ Primary WiFi failed. Trying fallback...");

    WiFi.disconnect(true, true);
    delay(300);

    Serial.print("Connecting to ");
    Serial.println(ssid2);

    WiFi.begin(ssid2, password2);

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
  }

  Serial.println("\n✅ WiFi connected");
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

// ---------- MQTT Reconnect ----------
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(DEVICE_ID)) {
      Serial.println("✅ connected to broker");
    } else {
      Serial.print("❌ failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5s");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  setup_wifi();
  client.setServer(mqtt_server, 1883);

  // I2C bus
  Wire.begin(SDA_PIN, SCL_PIN);

  // Scan bus
  Serial.println("🔍 Scanning I2C bus...");
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.printf("✅ Found device at 0x%02X\n", addr);
      delay(10);
    }
  }

  // BMP280 init
  if (bmp.begin(0x76) || bmp.begin(0x77)) {
    Serial.println("✅ BMP280 detected");
    useBMP = true;
  } else {
    Serial.println("❌ No BMP280 found");
  }

  Serial.println("⏳ Sensors initialized...\n");
  delay(2000);
}

void loop() {
  // If WiFi drops, reconnect (tries primary then fallback)
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("⚠️ WiFi disconnected. Reconnecting...");
    setup_wifi();
  }

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // --- MQ-9 Reading
  int mq9Value = analogRead(MQ9_AO);
  String mq9_status = "Safe";
  if (mq9Value > 2200) mq9_status = "LPG High";
  else if (mq9Value > 1200) mq9_status = "LPG Moderate";
  if (mq9Value > 2800) mq9_status += " | CO Dangerous";
  else if (mq9Value > 1600) mq9_status += " | CO Elevated";

  // --- MQ-135 Reading
  int mq135Value = analogRead(MQ135_AO);

  // --- BMP280 Reading
  float bmp_temp = NAN, bmp_press = NAN, bmp_alt = NAN;
  if (useBMP) {
    bmp_temp = bmp.readTemperature();
    bmp_press = bmp.readPressure() / 100.0F;
    bmp_alt = bmp.readAltitude(SEALEVELPRESSURE_HPA);
  }

  // --- Debug to Serial
  Serial.println("===== MQ-9 Gas Sensor =====");
  Serial.printf("Analog: %d | Status: %s\n", mq9Value, mq9_status.c_str());
  Serial.printf("===== MQ-135 Air Quality: %d =====\n", mq135Value);
  if (useBMP) {
    Serial.printf("BMP280 Temp: %.2f °C\n", bmp_temp);
    Serial.printf("BMP280 Pressure: %.2f hPa\n", bmp_press);
    Serial.printf("BMP280 Altitude: %.2f m\n", bmp_alt);
  }

  // --- Build JSON payload
  char payload[256];
  snprintf(payload, sizeof(payload),
    "{\"device\":\"%s\",\"mq9\":%d,\"mq9_status\":\"%s\",\"mq135\":%d,"
    "\"bmp_temp\":%.2f,\"bmp_press\":%.2f,\"bmp_alt\":%.2f}",
    DEVICE_ID,
    mq9Value,
    mq9_status.c_str(),
    mq135Value,
    isnan(bmp_temp) ? 0.0 : bmp_temp,
    isnan(bmp_press) ? 0.0 : bmp_press,
    isnan(bmp_alt) ? 0.0 : bmp_alt
  );

  // --- Publish to MQTT
  client.publish("hab/internal1c", payload);
  Serial.print("📤 Published: ");
  Serial.println(payload);

  delay(2000);
}
