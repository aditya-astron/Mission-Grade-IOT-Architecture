#pragma once

#include <cstdint>

namespace habitat {

struct Bmp280Calib {
    uint16_t dig_T1;
    int16_t dig_T2;
    int16_t dig_T3;
    uint16_t dig_P1;
    int16_t dig_P2;
    int16_t dig_P3;
    int16_t dig_P4;
    int16_t dig_P5;
    int16_t dig_P6;
    int16_t dig_P7;
    int16_t dig_P8;
    int16_t dig_P9;
};

struct Bmp280Reading {
    float temp_c;
    float pressure_pa;
    int32_t t_fine;
    bool ok;
};

// Bosch BMP280 integer compensation (datasheet rev 1.19).
// adc_T / adc_P are 20-bit uncompensated values.
bool bmp280_parse_calib(const uint8_t calib[24], Bmp280Calib* out);
int32_t bmp280_compensate_t_fine(int32_t adc_t, const Bmp280Calib& c);
float bmp280_temp_c(int32_t t_fine);
float bmp280_pressure_pa(int32_t adc_p, int32_t t_fine, const Bmp280Calib& c);
float bmp280_altitude_m(float pressure_pa, float sea_level_hpa);
Bmp280Reading bmp280_compensate(int32_t adc_t, int32_t adc_p, const Bmp280Calib& c);

struct Dht11Reading {
    float humidity_rh;
    float temp_c;
    bool ok;
};

// 40-bit DHT11 frame: hum_int, hum_dec, temp_int, temp_dec, checksum.
Dht11Reading dht11_decode_bytes(const uint8_t raw[5]);
// high_us[40] are the high-pulse widths after each 50 us low.
Dht11Reading dht11_decode_pulses(const uint16_t high_us[40], uint16_t zero_max_us = 40);

enum class Mq9Class : uint8_t { Safe = 0, LpgModerate = 1, LpgHigh = 2, CoDanger = 3 };

struct Mq9Status {
    Mq9Class cls;
    bool lpg_moderate;
    bool lpg_high;
    bool co_elevated;
    bool co_danger;
};

Mq9Status mq9_classify(int raw);
const char* mq9_status_text(const Mq9Status& s);
int mq135_band(int raw); // 0 ok, 1 warning, 2 alarm

float anemometer_mps(float volts);
float anemometer_mps_from_adc(int raw, int adc_max, float vref);

struct GeigerDose {
    float cpm;
    float usv_h;
};

GeigerDose geiger_dose(uint32_t pulses, uint32_t window_ms, float usv_per_cpm);

struct Scd4xReading {
    uint16_t co2_ppm;
    float temp_c;
    float humidity_rh;
    bool ok;
};

// 9-byte SCD4x measurement: 3 * (MSB LSB CRC).
Scd4xReading scd4x_parse(const uint8_t raw[9]);
uint16_t bno055_u16_le(uint8_t lo, uint8_t hi);
float bno055_accel_ms2(int16_t lsb); // 1 m/s^2 = 100 LSB in m/s^2 units
float accel_magnitude(float x, float y, float z);

} // namespace habitat
