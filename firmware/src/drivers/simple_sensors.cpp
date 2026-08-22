#include "habitat/drivers.h"

#include "habitat/config.h"

namespace habitat {

Dht11Driver::Dht11Driver(const Hal* hal, int pin) : hal_(hal), pin_(pin) {}

bool Dht11Driver::read(Dht11Reading* out) {
    if (!hal_ || out == nullptr) {
        return false;
    }
    const HalGpio& g = hal_->gpio;
    if (!g.mode || !g.write || !g.read) {
        return false;
    }

    g.mode(g.ctx, pin_, GpioMode::Output);
    g.write(g.ctx, pin_, 0);
    hal_delay_us(*hal_, 20000);
    g.write(g.ctx, pin_, 1);
    hal_delay_us(*hal_, 30);
    g.mode(g.ctx, pin_, GpioMode::InputPullup);

    // Wait for sensor response: 80 us low, 80 us high.
    uint32_t spins = 0;
    while (g.read(g.ctx, pin_) == 1) {
        if (++spins > 20000) {
            return false;
        }
        hal_delay_us(*hal_, 1);
    }
    spins = 0;
    while (g.read(g.ctx, pin_) == 0) {
        if (++spins > 200) {
            return false;
        }
        hal_delay_us(*hal_, 1);
    }
    spins = 0;
    while (g.read(g.ctx, pin_) == 1) {
        if (++spins > 200) {
            return false;
        }
        hal_delay_us(*hal_, 1);
    }

    uint16_t high_us[40]{};
    for (int i = 0; i < 40; ++i) {
        spins = 0;
        while (g.read(g.ctx, pin_) == 0) {
            if (++spins > 200) {
                return false;
            }
            hal_delay_us(*hal_, 1);
        }
        uint16_t high = 0;
        while (g.read(g.ctx, pin_) == 1) {
            if (++high > 200) {
                return false;
            }
            hal_delay_us(*hal_, 1);
        }
        high_us[i] = high;
    }
    *out = dht11_decode_pulses(high_us, 40);
    return out->ok;
}

AnalogSensor::AnalogSensor(const Hal* hal, int pin) : hal_(hal), pin_(pin) {}

int AnalogSensor::read_raw() const {
    if (!hal_ || !hal_->adc.read_raw) {
        return 0;
    }
    return hal_->adc.read_raw(hal_->adc.ctx, pin_);
}

float AnalogSensor::read_volts(float vref, int adc_max) const {
    if (adc_max <= 0) {
        return 0.0f;
    }
    return (static_cast<float>(read_raw()) / static_cast<float>(adc_max)) * vref;
}

GeigerCounter::GeigerCounter(const Hal* hal, int pin) : hal_(hal), pin_(pin) {}

bool GeigerCounter::begin() {
    if (!hal_ || !hal_->gpio.mode) {
        return false;
    }
    hal_->gpio.mode(hal_->gpio.ctx, pin_, GpioMode::InputPullup);
    if (hal_->gpio.attach_isr) {
        // Caller must bind the ISR to this instance; firmware main does that.
        return true;
    }
    return true;
}

void GeigerCounter::on_pulse() {
    ++total_;
}

GeigerDose GeigerCounter::sample(uint32_t now_ms, uint32_t window_ms, float usv_per_cpm) {
    const uint32_t total = total_;
    uint32_t dt = window_ms;
    uint32_t pulses = total;
    if (last_ms_ != 0) {
        dt = now_ms - last_ms_;
        pulses = total - last_total_;
    }
    last_ms_ = now_ms;
    last_total_ = total;
    if (dt == 0) {
        dt = window_ms;
    }
    return geiger_dose(pulses, dt, usv_per_cpm);
}

} // namespace habitat
