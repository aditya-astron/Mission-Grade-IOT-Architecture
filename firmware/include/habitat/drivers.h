#pragma once

#include "habitat/algo.h"
#include "habitat/hal.h"

#include <cstdint>

namespace habitat {

class Bmp280Driver {
  public:
    explicit Bmp280Driver(const Hal* hal, uint8_t addr = 0x76);
    bool probe();
    bool begin();
    bool read(Bmp280Reading* out);
    uint8_t address() const { return addr_; }
    const Bmp280Calib& calib() const { return calib_; }

  private:
    const Hal* hal_;
    uint8_t addr_;
    Bmp280Calib calib_{};
    bool ready_ = false;
    bool write_u8(uint8_t reg, uint8_t value);
    bool read_regs(uint8_t reg, uint8_t* data, size_t len);
};

class Bno055Driver {
  public:
    explicit Bno055Driver(const Hal* hal, uint8_t addr = 0x28);
    bool begin();
    bool read_accel(float* x, float* y, float* z);

  private:
    const Hal* hal_;
    uint8_t addr_;
    bool write_u8(uint8_t reg, uint8_t value);
    bool read_regs(uint8_t reg, uint8_t* data, size_t len);
};

class Scd4xDriver {
  public:
    explicit Scd4xDriver(const Hal* hal, uint8_t addr = 0x62);
    bool begin();
    bool read(Scd4xReading* out);

  private:
    const Hal* hal_;
    uint8_t addr_;
    bool command(uint16_t cmd);
};

class Dht11Driver {
  public:
    Dht11Driver(const Hal* hal, int pin);
    bool read(Dht11Reading* out);

  private:
    const Hal* hal_;
    int pin_;
};

class AnalogSensor {
  public:
    AnalogSensor(const Hal* hal, int pin);
    int read_raw() const;
    float read_volts(float vref, int adc_max) const;

  private:
    const Hal* hal_;
    int pin_;
};

class GeigerCounter {
  public:
    GeigerCounter(const Hal* hal, int pin);
    bool begin();
    void on_pulse();
    GeigerDose sample(uint32_t now_ms, uint32_t window_ms, float usv_per_cpm);
    uint32_t total() const { return total_; }

  private:
    const Hal* hal_;
    int pin_;
    volatile uint32_t total_ = 0;
    uint32_t last_total_ = 0;
    uint32_t last_ms_ = 0;
};

} // namespace habitat
