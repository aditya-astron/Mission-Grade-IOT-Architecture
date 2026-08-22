#ifdef ARDUINO

#include "habitat/config.h"
#include "habitat/hal.h"
#include "habitat/pins.h"

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

namespace habitat {
namespace {

bool i2c_write_fn(void*, uint8_t addr, const uint8_t* data, size_t len) {
    Wire.beginTransmission(addr);
    if (data != nullptr && len > 0) {
        Wire.write(data, static_cast<size_t>(len));
    }
    return Wire.endTransmission() == 0;
}

bool i2c_read_fn(void*, uint8_t addr, uint8_t* data, size_t len) {
    const size_t got = Wire.requestFrom(static_cast<int>(addr), static_cast<int>(len));
    if (got < len) {
        return false;
    }
    for (size_t i = 0; i < len; ++i) {
        data[i] = static_cast<uint8_t>(Wire.read());
    }
    return true;
}

bool i2c_write_read_fn(void* ctx, uint8_t addr, const uint8_t* w, size_t wl, uint8_t* r,
                       size_t rl) {
    return i2c_write_fn(ctx, addr, w, wl) && i2c_read_fn(ctx, addr, r, rl);
}

void spi_select_fn(void*, bool active) {
    digitalWrite(pins::kBmp280Cs, active ? LOW : HIGH);
}

void spi_transfer_fn(void*, const uint8_t* tx, uint8_t* rx, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        const uint8_t in = SPI.transfer(tx ? tx[i] : 0x00);
        if (rx) {
            rx[i] = in;
        }
    }
}

size_t uart_write_fn(void*, const uint8_t* data, size_t len) {
    return Serial.write(data, len);
}

void gpio_mode_fn(void*, int pin, GpioMode mode) {
    switch (mode) {
    case GpioMode::Output:
        pinMode(pin, OUTPUT);
        break;
    case GpioMode::InputPullup:
        pinMode(pin, INPUT_PULLUP);
        break;
    default:
        pinMode(pin, INPUT);
        break;
    }
}

void gpio_write_fn(void*, int pin, int level) {
    digitalWrite(pin, level ? HIGH : LOW);
}

int gpio_read_fn(void*, int pin) {
    return digitalRead(pin);
}

void gpio_isr_fn(void*, int pin, void (*isr)(), Edge edge) {
    int e = RISING;
    if (edge == Edge::Falling) {
        e = FALLING;
    } else if (edge == Edge::Change) {
        e = CHANGE;
    }
    attachInterrupt(digitalPinToInterrupt(pin), isr, e);
}

int adc_read_fn(void*, int pin) {
    return analogRead(pin);
}

uint32_t millis_fn(void*) {
    return millis();
}

void delay_us_fn(void*, uint32_t us) {
    delayMicroseconds(us);
}

} // namespace

bool hal_init_esp32(Hal* out) {
    if (out == nullptr) {
        return false;
    }
    Wire.begin(pins::kI2cSda, pins::kI2cScl);
    Wire.setClock(config::kI2cHz);
    pinMode(pins::kBmp280Cs, OUTPUT);
    digitalWrite(pins::kBmp280Cs, HIGH);
    SPI.begin(pins::kSpiSck, pins::kSpiMiso, pins::kSpiMosi, pins::kBmp280Cs);
    SPI.setFrequency(config::kSpiHz);

    out->i2c = HalI2c{i2c_write_fn, i2c_read_fn, i2c_write_read_fn, nullptr};
    out->spi = HalSpi{spi_select_fn, spi_transfer_fn, nullptr};
    out->uart = HalUart{uart_write_fn, nullptr};
    out->gpio = HalGpio{gpio_mode_fn, gpio_write_fn, gpio_read_fn, gpio_isr_fn, nullptr};
    out->adc = HalAdc{adc_read_fn, nullptr};
    out->clock = HalClock{millis_fn, delay_us_fn, nullptr};
    return true;
}

} // namespace habitat

#endif
