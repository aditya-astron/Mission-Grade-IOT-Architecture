#include "harness.hpp"

#include "habitat/algo.h"
#include "habitat/config.h"

#include <cstring>

void test_algo() {
    uint8_t calib[24] = {0x70, 0x6B, 0x43, 0x67, 0x18, 0xFC, 0x7D, 0x8E, 0x42, 0xD6, 0xD0, 0x0B,
                         0x27, 0x0B, 0x8C, 0x00, 0xF9, 0xFF, 0x8C, 0x3C, 0xF8, 0xC6, 0x70, 0x17};
    habitat::Bmp280Calib c{};
    EXPECT_TRUE(habitat::bmp280_parse_calib(calib, &c));
    EXPECT_EQ(c.dig_T1, 27504);
    EXPECT_EQ(c.dig_T2, 26435);
    EXPECT_EQ(c.dig_T3, -1000);
    EXPECT_EQ(c.dig_P1, 36477);

    const int32_t t_fine = habitat::bmp280_compensate_t_fine(519888, c);
    EXPECT_EQ(t_fine, 128422);
    EXPECT_NEAR(habitat::bmp280_temp_c(t_fine), 25.08, 0.001);

    const float pa = habitat::bmp280_pressure_pa(415148, t_fine, c);
    EXPECT_NEAR(pa, 100653.25, 0.5);
    const habitat::Bmp280Reading r = habitat::bmp280_compensate(519888, 415148, c);
    EXPECT_TRUE(r.ok);
    EXPECT_NEAR(r.temp_c, 25.08, 0.001);
    EXPECT_NEAR(habitat::bmp280_altitude_m(101325.0f, 1013.25f), 0.0, 1.0);

    const uint8_t dht_ok[5] = {45, 0, 23, 0, 68};
    const habitat::Dht11Reading d = habitat::dht11_decode_bytes(dht_ok);
    EXPECT_TRUE(d.ok);
    EXPECT_NEAR(d.humidity_rh, 45.0, 0.01);
    EXPECT_NEAR(d.temp_c, 23.0, 0.01);

    const uint8_t dht_bad[5] = {45, 0, 23, 0, 0};
    EXPECT_TRUE(!habitat::dht11_decode_bytes(dht_bad).ok);

    uint16_t pulses[40]{};
    // 45 = 00101101, 0, 23 = 00010111, 0, 68 = 01000100
    const uint8_t bits[5] = {45, 0, 23, 0, 68};
    for (int i = 0; i < 40; ++i) {
        const int bit = (bits[i / 8] >> (7 - (i % 8))) & 1;
        pulses[i] = bit ? 70 : 26;
    }
    const habitat::Dht11Reading dp = habitat::dht11_decode_pulses(pulses, 40);
    EXPECT_TRUE(dp.ok);
    EXPECT_NEAR(dp.temp_c, 23.0, 0.01);

    EXPECT_NEAR(habitat::anemometer_mps(0.033f), 0.0, 0.001);
    EXPECT_NEAR(habitat::anemometer_mps(3.3f), 32.4, 0.001);
    EXPECT_NEAR(habitat::anemometer_mps(1.6665f), 16.2, 0.05);
    EXPECT_NEAR(habitat::anemometer_mps(0.0f), 0.0, 0.001);
    EXPECT_NEAR(habitat::anemometer_mps_from_adc(4095, 4095, 3.3f), 32.4, 0.001);

    const habitat::GeigerDose g =
        habitat::geiger_dose(60, 60000, habitat::config::kGeigerUsvPerCpm);
    EXPECT_NEAR(g.cpm, 60.0, 0.001);
    EXPECT_NEAR(g.usv_h, 0.342, 0.001);
    const habitat::GeigerDose g2 =
        habitat::geiger_dose(120, 30000, habitat::config::kGeigerUsvPerCpm);
    EXPECT_NEAR(g2.cpm, 240.0, 0.001);

    const habitat::Mq9Status safe = habitat::mq9_classify(200);
    EXPECT_STREQ(habitat::mq9_status_text(safe), "safe");
    const habitat::Mq9Status mod = habitat::mq9_classify(1500);
    EXPECT_STREQ(habitat::mq9_status_text(mod), "lpg_moderate");
    EXPECT_TRUE(mod.co_elevated == false);
    const habitat::Mq9Status high = habitat::mq9_classify(2300);
    EXPECT_STREQ(habitat::mq9_status_text(high), "lpg_high");
    const habitat::Mq9Status dang = habitat::mq9_classify(3000);
    EXPECT_STREQ(habitat::mq9_status_text(dang), "co_danger");
    EXPECT_EQ(habitat::mq135_band(100), 0);
    EXPECT_EQ(habitat::mq135_band(1500), 1);
    EXPECT_EQ(habitat::mq135_band(3000), 2);

    const uint8_t scd[9] = {0x03, 0x20, 0x2A, 0x66, 0x66, 0x93, 0x80, 0x00, 0xA2};
    const habitat::Scd4xReading sc = habitat::scd4x_parse(scd);
    EXPECT_TRUE(sc.ok);
    EXPECT_EQ(sc.co2_ppm, 800);
    EXPECT_NEAR(sc.temp_c, 25.0, 0.02);
    EXPECT_NEAR(sc.humidity_rh, 50.0, 0.02);

    uint8_t scd_bad[9];
    std::memcpy(scd_bad, scd, 9);
    scd_bad[2] ^= 0x01;
    EXPECT_TRUE(!habitat::scd4x_parse(scd_bad).ok);

    EXPECT_NEAR(habitat::bno055_accel_ms2(981), 9.81, 0.001);
    EXPECT_NEAR(habitat::accel_magnitude(0, 0, 9.81f), 9.81, 0.001);
    EXPECT_EQ(habitat::bno055_u16_le(0xD5, 0x03), 981);
}
