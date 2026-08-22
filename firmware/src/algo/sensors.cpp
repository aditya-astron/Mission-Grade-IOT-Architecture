#include "habitat/algo.h"

#include "habitat/config.h"
#include "habitat/crc.h"

#include <cmath>
#include <cstdio>

namespace habitat {

Dht11Reading dht11_decode_bytes(const uint8_t raw[5]) {
    Dht11Reading r{};
    if (raw == nullptr) {
        return r;
    }
    const uint8_t sum = static_cast<uint8_t>(raw[0] + raw[1] + raw[2] + raw[3]);
    if (sum != raw[4]) {
        return r;
    }
    // DHT11: integer parts only. Decimal bytes are typically 0.
    r.humidity_rh = static_cast<float>(raw[0]) + static_cast<float>(raw[1]) * 0.1f;
    r.temp_c = static_cast<float>(raw[2]) + static_cast<float>(raw[3]) * 0.1f;
    if (r.humidity_rh > 100.0f || r.temp_c > 60.0f) {
        return r;
    }
    r.ok = true;
    return r;
}

Dht11Reading dht11_decode_pulses(const uint16_t high_us[40], uint16_t zero_max_us) {
    uint8_t raw[5]{};
    if (high_us == nullptr) {
        return {};
    }
    for (int i = 0; i < 40; ++i) {
        const int bit = (high_us[i] > zero_max_us) ? 1 : 0;
        raw[i / 8] = static_cast<uint8_t>((raw[i / 8] << 1) | bit);
    }
    return dht11_decode_bytes(raw);
}

Mq9Status mq9_classify(int raw) {
    Mq9Status s{};
    s.lpg_moderate = raw > config::kMq9LpgModerate;
    s.lpg_high = raw > config::kMq9LpgHigh;
    s.co_elevated = raw > config::kMq9CoElevated;
    s.co_danger = raw > config::kMq9CoDanger;
    if (s.co_danger) {
        s.cls = Mq9Class::CoDanger;
    } else if (s.lpg_high) {
        s.cls = Mq9Class::LpgHigh;
    } else if (s.lpg_moderate) {
        s.cls = Mq9Class::LpgModerate;
    } else {
        s.cls = Mq9Class::Safe;
    }
    return s;
}

const char* mq9_status_text(const Mq9Status& s) {
    if (s.cls == Mq9Class::CoDanger) {
        return "co_danger";
    }
    if (s.cls == Mq9Class::LpgHigh) {
        return "lpg_high";
    }
    if (s.cls == Mq9Class::LpgModerate) {
        return "lpg_moderate";
    }
    return "safe";
}

int mq135_band(int raw) {
    if (raw > config::kMq135Alarm) {
        return 2;
    }
    if (raw > config::kMq135Warning) {
        return 1;
    }
    return 0;
}

float anemometer_mps(float volts) {
    const float span = config::kAnemometerVmax - config::kAnemometerVmin;
    if (span <= 0.0f) {
        return 0.0f;
    }
    float v = (volts - config::kAnemometerVmin) / span * config::kAnemometerMpsMax;
    if (v < 0.0f) {
        v = 0.0f;
    }
    if (v > config::kAnemometerMpsMax) {
        v = config::kAnemometerMpsMax;
    }
    return v;
}

float anemometer_mps_from_adc(int raw, int adc_max, float vref) {
    if (adc_max <= 0) {
        return 0.0f;
    }
    const float volts = (static_cast<float>(raw) / static_cast<float>(adc_max)) * vref;
    return anemometer_mps(volts);
}

GeigerDose geiger_dose(uint32_t pulses, uint32_t window_ms, float usv_per_cpm) {
    GeigerDose d{};
    if (window_ms == 0) {
        return d;
    }
    d.cpm = (static_cast<float>(pulses) * 60000.0f) / static_cast<float>(window_ms);
    d.usv_h = d.cpm * usv_per_cpm;
    return d;
}

Scd4xReading scd4x_parse(const uint8_t raw[9]) {
    Scd4xReading r{};
    if (raw == nullptr) {
        return r;
    }
    for (int i = 0; i < 3; ++i) {
        if (crc8_sensirion(raw + i * 3, 2) != raw[i * 3 + 2]) {
            return r;
        }
    }
    const uint16_t co2 = static_cast<uint16_t>((raw[0] << 8) | raw[1]);
    const uint16_t t_raw = static_cast<uint16_t>((raw[3] << 8) | raw[4]);
    const uint16_t h_raw = static_cast<uint16_t>((raw[6] << 8) | raw[7]);
    r.co2_ppm = co2;
    r.temp_c = -45.0f + 175.0f * (static_cast<float>(t_raw) / 65535.0f);
    r.humidity_rh = 100.0f * (static_cast<float>(h_raw) / 65535.0f);
    r.ok = co2 != 0;
    return r;
}

uint16_t bno055_u16_le(uint8_t lo, uint8_t hi) {
    return static_cast<uint16_t>(lo | (static_cast<uint16_t>(hi) << 8));
}

float bno055_accel_ms2(int16_t lsb) {
    return static_cast<float>(lsb) / 100.0f;
}

float accel_magnitude(float x, float y, float z) {
    return std::sqrt(x * x + y * y + z * z);
}

} // namespace habitat
