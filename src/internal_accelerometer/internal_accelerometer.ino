#include "config.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>

// ---------- Wi-Fi + MQTT Config ----------
const char* ssid       = WIFI_SSID;     // 🔹 primary Wi-Fi
const char* password   = WIFI_PASSWORD;       // 🔹 primary Wi-Fi password

// 🔹 fallback Wi-Fi 
const char* ssid2      = WIFI_SSID_FALLBACK;
const char* password2  = WIFI_PASSWORD_FALLBACK;

// 🔹 Google Cloud VM External IP (Mosquitto broker)
const char* mqtt_server = MQTT_SERVER;

// Device identity
#define DEVICE_ID "internal1a"

// ---------- MQTT ----------
WiFiClient espClient;
PubSubClient client(espClient);

// ---------- I2C Pins for ESP32 ----------
#define SDA_PIN 21
#define SCL_PIN 22

// BNO055 IMU sensor object
Adafruit_BNO055 bno = Adafruit_BNO055(55);

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
  delay(1000);

  setup_wifi();
  client.setServer(mqtt_server, 1883);

  // Initialize I2C bus
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);

  // Initialize BNO055
  if (!bno.begin()) {
    Serial.println("⚠ BNO055 sensor NOT found! Halting.");
    while (1);
  }
  bno.setExtCrystalUse(true);
  Serial.println("✅ BNO055 sensor initialized");
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

  // -------- Read Sensors --------
  imu::Vector<3> euler = bno.getVector(Adafruit_BNO055::VECTOR_EULER);
  imu::Vector<3> accel = bno.getVector(Adafruit_BNO055::VECTOR_ACCELEROMETER);
  imu::Vector<3> gyro  = bno.getVector(Adafruit_BNO055::VECTOR_GYROSCOPE);
  imu::Vector<3> mag   = bno.getVector(Adafruit_BNO055::VECTOR_MAGNETOMETER);

  uint8_t sys, gyroCal, accelCal, magCal;
  bno.getCalibration(&sys, &gyroCal, &accelCal, &magCal);

  // -------- Build JSON (manual string) --------
  char payload[256];
  snprintf(payload, sizeof(payload),
    "{\"device\":\"%s\",\"euler\":[%.2f,%.2f,%.2f],\"accel\":[%.2f,%.2f,%.2f],"
    "\"gyro\":[%.2f,%.2f,%.2f],\"mag\":[%.2f,%.2f,%.2f],"
    "\"calibration\":[%d,%d,%d,%d]}",
    DEVICE_ID,
    euler.x(), euler.y(), euler.z(),
    accel.x(), accel.y(), accel.z(),
    gyro.x(), gyro.y(), gyro.z(),
    mag.x(), mag.y(), mag.z(),
    sys, gyroCal, accelCal, magCal
  );

  // -------- Publish to MQTT --------
  client.publish("hab/internal1a", payload);

  Serial.print("📤 Published: ");
  Serial.println(payload);

  delay(2000);
}
