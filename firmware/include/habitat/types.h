#pragma once

#include <cstddef>
#include <cstdint>

namespace habitat {

enum class NodeId : uint8_t {
    Unknown = 0,
    ExternalWeather = 1,
    ExternalRadiation = 2,
    InternalAir = 3,
    InternalImu = 4,
    InternalCo2 = 5,
};

enum class Channel : uint8_t {
    None = 0,
    TempC = 1,
    HumidityRh = 2,
    PressureHpa = 3,
    WindMps = 4,
    DoseRateUSvh = 5,
    Co2Ppm = 6,
    Mq9Raw = 7,
    Mq135Raw = 8,
    AccelMagMs2 = 9,
    Cpm = 10,
};

enum class Severity : uint8_t { Ok = 0, Warning = 1, Alarm = 2 };

enum class LogLevel : uint8_t { Debug = 0, Info = 1, Warn = 2, Error = 3 };

enum class MsgType : uint8_t {
    Telemetry = 1,
    Alert = 2,
    Log = 3,
    Heartbeat = 4,
};

struct Sample {
    Channel channel;
    float value;
    uint32_t timestamp_ms;
    bool valid;
};

inline const char* channel_name(Channel ch) {
    switch (ch) {
    case Channel::TempC:
        return "temp_c";
    case Channel::HumidityRh:
        return "humidity_rh";
    case Channel::PressureHpa:
        return "pressure_hpa";
    case Channel::WindMps:
        return "wind_mps";
    case Channel::DoseRateUSvh:
        return "dose_usv_h";
    case Channel::Co2Ppm:
        return "co2_ppm";
    case Channel::Mq9Raw:
        return "mq9_raw";
    case Channel::Mq135Raw:
        return "mq135_raw";
    case Channel::AccelMagMs2:
        return "accel_mag_ms2";
    case Channel::Cpm:
        return "cpm";
    default:
        return "unknown";
    }
}

inline const char* node_name(NodeId id) {
    switch (id) {
    case NodeId::ExternalWeather:
        return "external_weather";
    case NodeId::ExternalRadiation:
        return "external_radiation";
    case NodeId::InternalAir:
        return "internal_air";
    case NodeId::InternalImu:
        return "internal_imu";
    case NodeId::InternalCo2:
        return "internal_co2";
    default:
        return "unknown";
    }
}

inline const char* severity_name(Severity s) {
    switch (s) {
    case Severity::Alarm:
        return "alarm";
    case Severity::Warning:
        return "warning";
    default:
        return "ok";
    }
}

} // namespace habitat
