#pragma once

// Pin map taken from the habitat ESP32 boards that were already wired.
// Do not rename GPIOs here without updating docs/hardware.md.

namespace habitat {
namespace pins {

constexpr int kI2cSda = 21;
constexpr int kI2cScl = 22;
constexpr int kDht11Data = 4;
constexpr int kAnemometerAdc = 34;
constexpr int kGeigerPulse = 13;
constexpr int kActivityLed = 25;
constexpr int kAlertBuzzer = 26;
constexpr int kMq9Adc = 32;
constexpr int kMq135Adc = 33;
constexpr int kBmp280Cs = 5; // SPI optional path
constexpr int kSpiSck = 18;
constexpr int kSpiMiso = 19;
constexpr int kSpiMosi = 23;

} // namespace pins
} // namespace habitat
