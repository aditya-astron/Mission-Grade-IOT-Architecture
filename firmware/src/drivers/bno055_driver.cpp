#include "habitat/drivers.h"

namespace habitat {
namespace {
constexpr uint8_t kRegChipId = 0x00;
constexpr uint8_t kRegPageId = 0x07;
constexpr uint8_t kRegAccel = 0x08;
constexpr uint8_t kRegUnitSel = 0x3B;
constexpr uint8_t kRegOprMode = 0x3D;
constexpr uint8_t kRegPwrMode = 0x3E;
constexpr uint8_t kRegSysTrigger = 0x3F;
constexpr uint8_t kChipId = 0xA0;
constexpr uint8_t kModeConfig = 0x00;
constexpr uint8_t kModeNdof = 0x0C;
} // namespace

Bno055Driver::Bno055Driver(const Hal* hal, uint8_t addr) : hal_(hal), addr_(addr) {}

bool Bno055Driver::write_u8(uint8_t reg, uint8_t value) {
    const uint8_t buf[2] = {reg, value};
    return hal_ && i2c_write(*hal_, addr_, buf, 2);
}

bool Bno055Driver::read_regs(uint8_t reg, uint8_t* data, size_t len) {
    return hal_ && i2c_write_read(*hal_, addr_, &reg, 1, data, len);
}

bool Bno055Driver::begin() {
    uint8_t id = 0;
    if (!read_regs(kRegChipId, &id, 1) || id != kChipId) {
        return false;
    }
    if (!write_u8(kRegOprMode, kModeConfig)) {
        return false;
    }
    if (hal_) {
        hal_delay_us(*hal_, 20000);
    }
    if (!write_u8(kRegPwrMode, 0x00)) {
        return false;
    }
    if (!write_u8(kRegPageId, 0x00)) {
        return false;
    }
    if (!write_u8(kRegSysTrigger, 0x00)) {
        return false;
    }
    // Accel in m/s^2 (bit 0 = 0), Celsius.
    if (!write_u8(kRegUnitSel, 0x00)) {
        return false;
    }
    if (!write_u8(kRegOprMode, kModeNdof)) {
        return false;
    }
    if (hal_) {
        hal_delay_us(*hal_, 8000);
    }
    return true;
}

bool Bno055Driver::read_accel(float* x, float* y, float* z) {
    uint8_t raw[6];
    if (!read_regs(kRegAccel, raw, sizeof(raw))) {
        return false;
    }
    const int16_t xi = static_cast<int16_t>(bno055_u16_le(raw[0], raw[1]));
    const int16_t yi = static_cast<int16_t>(bno055_u16_le(raw[2], raw[3]));
    const int16_t zi = static_cast<int16_t>(bno055_u16_le(raw[4], raw[5]));
    if (x) {
        *x = bno055_accel_ms2(xi);
    }
    if (y) {
        *y = bno055_accel_ms2(yi);
    }
    if (z) {
        *z = bno055_accel_ms2(zi);
    }
    return true;
}

} // namespace habitat
