#include "harness.hpp"

#include "habitat/drivers.h"

#include <cstring>
#include <map>
#include <vector>

namespace {

struct MockI2c {
    std::map<uint8_t, std::vector<uint8_t>> regs;  // last written register -> canned read
    std::map<uint16_t, std::vector<uint8_t>> cmds; // 16-bit command -> canned read
    uint8_t last_reg = 0;
    uint16_t last_cmd = 0;
    bool fail = false;
};

bool mock_write(void* ctx, uint8_t, const uint8_t* data, size_t len) {
    auto* m = static_cast<MockI2c*>(ctx);
    if (m->fail) {
        return false;
    }
    if (data == nullptr || len == 0) {
        return true;
    }
    if (len == 1) {
        m->last_reg = data[0];
        return true;
    }
    if (len == 2) {
        const uint16_t cmd = static_cast<uint16_t>((data[0] << 8) | data[1]);
        if (m->cmds.find(cmd) != m->cmds.end()) {
            m->last_cmd = cmd;
            return true;
        }
        m->last_reg = data[0];
        m->regs[data[0]] = std::vector<uint8_t>(data + 1, data + len);
        return true;
    }
    return true;
}

bool mock_read(void* ctx, uint8_t, uint8_t* data, size_t len) {
    auto* m = static_cast<MockI2c*>(ctx);
    if (m->fail) {
        return false;
    }
    auto it = m->cmds.find(m->last_cmd);
    if (it != m->cmds.end() && it->second.size() >= len) {
        std::memcpy(data, it->second.data(), len);
        return true;
    }
    auto ir = m->regs.find(m->last_reg);
    if (ir != m->regs.end() && ir->second.size() >= len) {
        std::memcpy(data, ir->second.data(), len);
        return true;
    }
    return false;
}

bool mock_write_read(void* ctx, uint8_t addr, const uint8_t* w, size_t wl, uint8_t* r, size_t rl) {
    return mock_write(ctx, addr, w, wl) && mock_read(ctx, addr, r, rl);
}

uint32_t mock_millis(void*) {
    return 0;
}
void mock_delay(void*, uint32_t) {}

habitat::Hal make_hal(MockI2c* m) {
    habitat::Hal h{};
    h.i2c = habitat::HalI2c{mock_write, mock_read, mock_write_read, m};
    h.clock = habitat::HalClock{mock_millis, mock_delay, nullptr};
    return h;
}

} // namespace

void test_drivers() {
    MockI2c bus;
    // BMP280 chip id, calib, raw T/P.
    bus.regs[0xD0] = {0x58};
    bus.regs[0x88] = {0x70, 0x6B, 0x43, 0x67, 0x18, 0xFC, 0x7D, 0x8E, 0x42, 0xD6, 0xD0, 0x0B,
                      0x27, 0x0B, 0x8C, 0x00, 0xF9, 0xFF, 0x8C, 0x3C, 0xF8, 0xC6, 0x70, 0x17};
    // adc_p=415148 -> 65 5A C0, adc_t=519888 -> 7E ED 00
    bus.regs[0xF7] = {0x65, 0x5A, 0xC0, 0x7E, 0xED, 0x00};

    habitat::Hal hal = make_hal(&bus);
    habitat::Bmp280Driver bmp(&hal, 0x76);
    EXPECT_TRUE(bmp.probe());
    EXPECT_TRUE(bmp.begin());
    habitat::Bmp280Reading rd{};
    EXPECT_TRUE(bmp.read(&rd));
    EXPECT_NEAR(rd.temp_c, 25.08, 0.01);
    EXPECT_NEAR(rd.pressure_pa, 100653.25, 1.0);

    bus.regs[0xD0] = {0x00};
    habitat::Bmp280Driver missing(&hal, 0x76);
    EXPECT_TRUE(!missing.probe());

    MockI2c imu;
    imu.regs[0x00] = {0xA0};
    imu.regs[0x08] = {0x00, 0x00, 0x00, 0x00, 0xD5, 0x03}; // z = 981 LSB = 9.81 m/s^2
    habitat::Hal imu_hal = make_hal(&imu);
    habitat::Bno055Driver bno(&imu_hal, 0x28);
    EXPECT_TRUE(bno.begin());
    float x = 0, y = 0, z = 0;
    EXPECT_TRUE(bno.read_accel(&x, &y, &z));
    EXPECT_NEAR(x, 0.0, 0.001);
    EXPECT_NEAR(z, 9.81, 0.001);

    MockI2c scd;
    scd.cmds[0x3F86] = {};
    scd.cmds[0x21B1] = {};
    scd.cmds[0xEC05] = {0x03, 0x20, 0x2A, 0x66, 0x66, 0x93, 0x80, 0x00, 0xA2};
    habitat::Hal scd_hal = make_hal(&scd);
    habitat::Scd4xDriver scd4x(&scd_hal, 0x62);
    EXPECT_TRUE(scd4x.begin());
    habitat::Scd4xReading s{};
    EXPECT_TRUE(scd4x.read(&s));
    EXPECT_EQ(s.co2_ppm, 800);
    EXPECT_NEAR(s.temp_c, 25.0, 0.02);

    // Analog / geiger via mock ADC + software pulses.
    struct AdcCtx {
        int raw;
    } adc{2048};
    habitat::Hal analog{};
    analog.adc.read_raw = [](void* ctx, int) { return static_cast<AdcCtx*>(ctx)->raw; };
    analog.adc.ctx = &adc;
    analog.gpio.mode = [](void*, int, habitat::GpioMode) {};
    analog.clock.millis = mock_millis;
    analog.clock.delay_us = mock_delay;
    habitat::AnalogSensor anem(&analog, 34);
    EXPECT_EQ(anem.read_raw(), 2048);
    habitat::GeigerCounter geiger(&analog, 13);
    EXPECT_TRUE(geiger.begin());
    for (int i = 0; i < 60; ++i) {
        geiger.on_pulse();
    }
    const habitat::GeigerDose dose = geiger.sample(60000, 60000, 0.0057f);
    EXPECT_NEAR(dose.cpm, 60.0, 0.01);
}
