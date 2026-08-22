#include "habitat/drivers.h"

namespace habitat {
namespace {
constexpr uint16_t kCmdStartPeriodic = 0x21B1;
constexpr uint16_t kCmdReadMeasure = 0xEC05;
constexpr uint16_t kCmdStopPeriodic = 0x3F86;
} // namespace

Scd4xDriver::Scd4xDriver(const Hal* hal, uint8_t addr) : hal_(hal), addr_(addr) {}

bool Scd4xDriver::command(uint16_t cmd) {
    const uint8_t buf[2] = {static_cast<uint8_t>(cmd >> 8), static_cast<uint8_t>(cmd & 0xFF)};
    return hal_ && i2c_write(*hal_, addr_, buf, 2);
}

bool Scd4xDriver::begin() {
    if (!command(kCmdStopPeriodic)) {
        return false;
    }
    if (hal_) {
        hal_delay_us(*hal_, 500000);
    }
    return command(kCmdStartPeriodic);
}

bool Scd4xDriver::read(Scd4xReading* out) {
    if (out == nullptr || !command(kCmdReadMeasure)) {
        return false;
    }
    if (hal_) {
        hal_delay_us(*hal_, 1000);
    }
    uint8_t raw[9];
    if (!i2c_read(*hal_, addr_, raw, sizeof(raw))) {
        return false;
    }
    *out = scd4x_parse(raw);
    return out->ok;
}

} // namespace habitat
