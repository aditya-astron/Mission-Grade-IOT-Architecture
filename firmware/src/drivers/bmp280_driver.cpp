#include "habitat/drivers.h"

#include "habitat/config.h"

namespace habitat {
namespace {
constexpr uint8_t kRegId = 0xD0;
constexpr uint8_t kRegReset = 0xE0;
constexpr uint8_t kRegCtrlMeas = 0xF4;
constexpr uint8_t kRegConfig = 0xF5;
constexpr uint8_t kRegCalib = 0x88;
constexpr uint8_t kRegPress = 0xF7;
constexpr uint8_t kChipId = 0x58;
constexpr uint8_t kSoftReset = 0xB6;
} // namespace

Bmp280Driver::Bmp280Driver(const Hal* hal, uint8_t addr) : hal_(hal), addr_(addr) {}

bool Bmp280Driver::write_u8(uint8_t reg, uint8_t value) {
    const uint8_t buf[2] = {reg, value};
    return hal_ && i2c_write(*hal_, addr_, buf, 2);
}

bool Bmp280Driver::read_regs(uint8_t reg, uint8_t* data, size_t len) {
    return hal_ && i2c_write_read(*hal_, addr_, &reg, 1, data, len);
}

bool Bmp280Driver::probe() {
    uint8_t id = 0;
    if (!read_regs(kRegId, &id, 1)) {
        return false;
    }
    return id == kChipId;
}

bool Bmp280Driver::begin() {
    if (!probe()) {
        return false;
    }
    if (!write_u8(kRegReset, kSoftReset)) {
        return false;
    }
    if (hal_) {
        // Datasheet: 2 ms after soft reset.
        hal_delay_us(*hal_, 3000);
    }
    uint8_t calib[24];
    if (!read_regs(kRegCalib, calib, sizeof(calib))) {
        return false;
    }
    if (!bmp280_parse_calib(calib, &calib_)) {
        return false;
    }
    // osrs_t = x2, osrs_p = x16, normal mode. t_sb = 0.5 ms, filter off.
    if (!write_u8(kRegConfig, 0x00)) {
        return false;
    }
    if (!write_u8(kRegCtrlMeas, 0x57)) {
        return false;
    }
    ready_ = true;
    return true;
}

bool Bmp280Driver::read(Bmp280Reading* out) {
    if (!ready_ || out == nullptr) {
        return false;
    }
    uint8_t raw[6];
    if (!read_regs(kRegPress, raw, sizeof(raw))) {
        return false;
    }
    const int32_t adc_p = (static_cast<int32_t>(raw[0]) << 12) |
                          (static_cast<int32_t>(raw[1]) << 4) | (static_cast<int32_t>(raw[2]) >> 4);
    const int32_t adc_t = (static_cast<int32_t>(raw[3]) << 12) |
                          (static_cast<int32_t>(raw[4]) << 4) | (static_cast<int32_t>(raw[5]) >> 4);
    *out = bmp280_compensate(adc_t, adc_p, calib_);
    return out->ok;
}

} // namespace habitat
