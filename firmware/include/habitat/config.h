#pragma once

#include "habitat/types.h"

namespace habitat {
namespace config {

#ifndef HABITAT_NODE
#define HABITAT_NODE 1
#endif

#ifndef HABITAT_UART_BAUD
#define HABITAT_UART_BAUD 115200
#endif

#ifndef HABITAT_I2C_HZ
#define HABITAT_I2C_HZ 100000
#endif

#ifndef HABITAT_SPI_HZ
#define HABITAT_SPI_HZ 500000
#endif

constexpr NodeId kNode = static_cast<NodeId>(HABITAT_NODE);
constexpr uint32_t kUartBaud = HABITAT_UART_BAUD;
constexpr uint32_t kI2cHz = HABITAT_I2C_HZ;
constexpr uint32_t kSpiHz = HABITAT_SPI_HZ;

constexpr uint32_t kSamplePeriodMs = 2000;
constexpr uint32_t kAlertPeriodMs = 500;
constexpr uint32_t kTelemetryPeriodMs = 3000;
constexpr uint32_t kHeartbeatPeriodMs = 10000;
constexpr uint32_t kGeigerWindowMs = 60000;

// DHT11 requires >= 2 s between conversions.
constexpr uint32_t kDht11MinPeriodMs = 2000;
// SCD4x periodic measurement is ~5 s.
constexpr uint32_t kScd4xPeriodMs = 5000;

constexpr float kSeaLevelHpa = 1013.25f;

// Anemometer (analog, 0-3.3 V span used on the habitat boards).
constexpr float kAnemometerVmin = 0.033f;
constexpr float kAnemometerVmax = 3.3f;
constexpr float kAnemometerMpsMax = 32.4f;
constexpr float kAdcFullScaleV = 3.3f;
constexpr int kAdcMaxRaw = 4095;

// Geiger tube conversion is tube-specific. 0.0057 uSv/h per CPM is the
// factor used on the existing habitat boards (typical SBM-20 class).
constexpr float kGeigerUsvPerCpm = 0.0057f;

// MQ-series: raw 12-bit ADC bands from the original board bring-up.
// These are not ppm. Convert to ppm only after Ro calibration in clean air.
constexpr int kMq9LpgModerate = 1200;
constexpr int kMq9LpgHigh = 2200;
constexpr int kMq9CoElevated = 1600;
constexpr int kMq9CoDanger = 2800;
constexpr int kMq135Warning = 1200;
constexpr int kMq135Alarm = 2200;

// Habitat comfort / caution bands. Tune per analog-mission procedure.
constexpr float kTempWarnLowC = 15.0f;
constexpr float kTempWarnHighC = 30.0f;
constexpr float kTempAlarmLowC = 10.0f;
constexpr float kTempAlarmHighC = 35.0f;
constexpr float kRhWarnLow = 25.0f;
constexpr float kRhWarnHigh = 70.0f;
constexpr float kRhAlarmLow = 15.0f;
constexpr float kRhAlarmHigh = 80.0f;
constexpr float kPressWarnLowHpa = 980.0f;
constexpr float kPressWarnHighHpa = 1040.0f;
constexpr float kPressAlarmLowHpa = 950.0f;
constexpr float kPressAlarmHighHpa = 1060.0f;
constexpr float kWindWarnMps = 10.0f;
constexpr float kWindAlarmMps = 20.0f;
constexpr float kDoseWarnUsvh = 0.5f;
constexpr float kDoseAlarmUsvh = 2.0f;
constexpr float kCo2WarnPpm = 1000.0f;
constexpr float kCo2AlarmPpm = 2000.0f;
constexpr float kAccelWarnMs2 = 15.0f;
constexpr float kAccelAlarmMs2 = 25.0f;

// Hysteresis and debounce for the threshold engine.
constexpr float kHysteresisFrac = 0.05f;
constexpr uint8_t kDebounceHits = 2;

constexpr uint8_t kProtocolVersion = 1;
constexpr uint8_t kSof0 = 0xA5;
constexpr uint8_t kSof1 = 0x5A;
constexpr size_t kMaxPayload = 200;
constexpr size_t kMaxFrame = 8 + kMaxPayload + 2;

constexpr int kBmp280AddrPrimary = 0x76;
constexpr int kBmp280AddrSecondary = 0x77;
constexpr int kBno055Addr = 0x28;
constexpr int kScd4xAddr = 0x62;

} // namespace config
} // namespace habitat
