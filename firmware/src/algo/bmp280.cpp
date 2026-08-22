#include "habitat/algo.h"

#include "habitat/config.h"

#include <cmath>
#include <cstdint>

namespace habitat {

bool bmp280_parse_calib(const uint8_t calib[24], Bmp280Calib* out) {
    if (calib == nullptr || out == nullptr) {
        return false;
    }
    auto u16 = [&](int i) -> uint16_t {
        return static_cast<uint16_t>(calib[i] | (static_cast<uint16_t>(calib[i + 1]) << 8));
    };
    auto s16 = [&](int i) -> int16_t { return static_cast<int16_t>(u16(i)); };
    out->dig_T1 = u16(0);
    out->dig_T2 = s16(2);
    out->dig_T3 = s16(4);
    out->dig_P1 = u16(6);
    out->dig_P2 = s16(8);
    out->dig_P3 = s16(10);
    out->dig_P4 = s16(12);
    out->dig_P5 = s16(14);
    out->dig_P6 = s16(16);
    out->dig_P7 = s16(18);
    out->dig_P8 = s16(20);
    out->dig_P9 = s16(22);
    return out->dig_T1 != 0 && out->dig_P1 != 0;
}

int32_t bmp280_compensate_t_fine(int32_t adc_t, const Bmp280Calib& c) {
    const int32_t var1 = ((((adc_t >> 3) - (static_cast<int32_t>(c.dig_T1) << 1))) *
                          static_cast<int32_t>(c.dig_T2)) >>
                         11;
    const int32_t var2 = (((((adc_t >> 4) - static_cast<int32_t>(c.dig_T1)) *
                            ((adc_t >> 4) - static_cast<int32_t>(c.dig_T1))) >>
                           12) *
                          static_cast<int32_t>(c.dig_T3)) >>
                         14;
    return var1 + var2;
}

float bmp280_temp_c(int32_t t_fine) {
    const int32_t t = (t_fine * 5 + 128) >> 8;
    return static_cast<float>(t) / 100.0f;
}

float bmp280_pressure_pa(int32_t adc_p, int32_t t_fine, const Bmp280Calib& c) {
    int64_t var1 = static_cast<int64_t>(t_fine) - 128000;
    int64_t var2 = var1 * var1 * static_cast<int64_t>(c.dig_P6);
    var2 = var2 + ((var1 * static_cast<int64_t>(c.dig_P5)) << 17);
    var2 = var2 + (static_cast<int64_t>(c.dig_P4) << 35);
    var1 = ((var1 * var1 * static_cast<int64_t>(c.dig_P3)) >> 8) +
           ((var1 * static_cast<int64_t>(c.dig_P2)) << 12);
    var1 = (((((static_cast<int64_t>(1) << 47) + var1)) * static_cast<int64_t>(c.dig_P1)) >> 33);
    if (var1 == 0) {
        return 0.0f;
    }
    int64_t p = 1048576 - adc_p;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (static_cast<int64_t>(c.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (static_cast<int64_t>(c.dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (static_cast<int64_t>(c.dig_P7) << 4);
    return static_cast<float>(p) / 256.0f;
}

float bmp280_altitude_m(float pressure_pa, float sea_level_hpa) {
    const float sea_pa = sea_level_hpa * 100.0f;
    if (pressure_pa <= 0.0f || sea_pa <= 0.0f) {
        return 0.0f;
    }
    return 44330.0f * (1.0f - std::pow(pressure_pa / sea_pa, 0.1903f));
}

Bmp280Reading bmp280_compensate(int32_t adc_t, int32_t adc_p, const Bmp280Calib& c) {
    Bmp280Reading r{};
    r.t_fine = bmp280_compensate_t_fine(adc_t, c);
    r.temp_c = bmp280_temp_c(r.t_fine);
    r.pressure_pa = bmp280_pressure_pa(adc_p, r.t_fine, c);
    r.ok = r.pressure_pa > 0.0f;
    return r;
}

} // namespace habitat
