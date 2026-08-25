#include "config.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include "SensirionI2cScd4x.h"
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

// ---------- Wi-Fi + MQTT Config ----------
const char* ssid       = WIFI_SSID;     // 🔹 primary Wi-Fi
const char* password   = WIFI_PASSWORD;       // 🔹 primary Wi-Fi password

// 🔹 fallback Wi-Fi 
const char* ssid2      = WIFI_SSID_FALLBACK;
const char* password2  = WIFI_PASSWORD_FALLBACK;

// 🔹 Google Cloud VM External IP (Mosquitto broker)
const char* mqtt_server = MQTT_SERVER;

// Device identity
#define DEVICE_ID "internal1b"

// ---------- MQTT ----------
WiFiClient espClient;
PubSubClient client(espClient);

// ---------- Sensors ----------
#define DHTPIN 4       // DHT11 pin
#define DHTTYPE DHT11

SensirionI2cScd4x scd4x;
DHT dht(DHTPIN, DHTTYPE);

bool scd40_working = false;

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
  delay(3000);

  setup_wifi();
  client.setServer(mqtt_server, 1883);

  // --- DHT11 Setup
  dht.begin();
  Serial.println("🌡 DHT11 initialized");
  delay(2000);

  // --- I2C Setup
  Wire.begin(21, 22);
  Wire.setClock(10000);
  Wire.setTimeout(15000);
  Serial.println("📡 I2C initialized at 10kHz");

  // --- SCD40 Setup
  Serial.println("🌍 Initializing SCD40...");
  for (int attempt = 1; attempt <= 5; attempt++) {
    scd4x.begin(Wire, 0x62);
    delay(3000);

    uint16_t error = scd4x.stopPeriodicMeasurement();
    delay(2000);
    error = scd4x.startPeriodicMeasurement();

    if (error == 0) {
      Serial.println("✅ SCD40 SUCCESS!");
      scd40_working = true;
      break;
    } else {
      Serial.print("❌ SCD40 Error: "); Serial.println(error);
      delay(3000);
    }
  }
  if (!scd40_working) {
    Serial.println("❌ SCD40 failed - continuing with DHT11 only");
  }

  Serial.println("⏳ Waiting 10s for stabilization...");
  delay(10000);
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

  // Data placeholders
  uint16_t co2 = 0;
  float temp_scd40 = NAN, hum_scd40 = NAN;
  float dht_h = NAN, dht_t = NAN;

  // --- Read SCD40
  if (scd40_working) {
    uint16_t error = scd4x.readMeasurement(co2, temp_scd40, hum_scd40);
    if (error || co2 == 0) {
      Serial.println("❌ SCD40 read failed / waiting...");
    } else {
      Serial.printf("🌍 SCD40 CO2: %u ppm | 🌡 %.1f °C | 💧 %.1f %%\n",
                    co2, temp_scd40, hum_scd40);
    }
  }

  // --- Read DHT11
  dht_h = dht.readHumidity();
  dht_t = dht.readTemperature();
  if (isnan(dht_h) || isnan(dht_t)) {
    Serial.println("❌ Failed to read DHT11");
  } else {
    Serial.printf("💧 DHT11 Humidity: %.1f %% | 🌡 Temp: %.1f °C\n", dht_h, dht_t);
  }

  // --- Build JSON payload
  char payload[256];
  snprintf(payload, sizeof(payload),
    "{\"device\":\"%s\",\"co2\":%u,\"temp_scd40\":%.1f,\"hum_scd40\":%.1f,"
    "\"temp_dht11\":%.1f,\"hum_dht11\":%.1f}",
    DEVICE_ID,
    co2,
    isnan(temp_scd40) ? 0.0 : temp_scd40,
    isnan(hum_scd40) ? 0.0 : hum_scd40,
    isnan(dht_t) ? 0.0 : dht_t,
    isnan(dht_h) ? 0.0 : dht_h
  );

  // --- Publish to MQTT
  client.publish("hab/internal1b", payload);
  Serial.print("📤 Published: ");
  Serial.println(payload);

  delay(10000); // 10s cycle
}
