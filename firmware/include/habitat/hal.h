#pragma once

#include <cstddef>
#include <cstdint>

namespace habitat {

enum class GpioMode : uint8_t { Input = 0, InputPullup = 1, Output = 2 };
enum class Edge : uint8_t { Rising = 1, Falling = 2, Change = 3 };

struct HalI2c {
    bool (*write)(void* ctx, uint8_t addr, const uint8_t* data, size_t len);
    bool (*read)(void* ctx, uint8_t addr, uint8_t* data, size_t len);
    bool (*write_read)(void* ctx, uint8_t addr, const uint8_t* w, size_t wl, uint8_t* r, size_t rl);
    void* ctx;
};

struct HalSpi {
    void (*select)(void* ctx, bool active);
    void (*transfer)(void* ctx, const uint8_t* tx, uint8_t* rx, size_t len);
    void* ctx;
};

struct HalUart {
    size_t (*write)(void* ctx, const uint8_t* data, size_t len);
    void* ctx;
};

struct HalGpio {
    void (*mode)(void* ctx, int pin, GpioMode mode);
    void (*write)(void* ctx, int pin, int level);
    int (*read)(void* ctx, int pin);
    void (*attach_isr)(void* ctx, int pin, void (*isr)(), Edge edge);
    void* ctx;
};

struct HalAdc {
    int (*read_raw)(void* ctx, int pin);
    void* ctx;
};

struct HalClock {
    uint32_t (*millis)(void* ctx);
    void (*delay_us)(void* ctx, uint32_t us);
    void* ctx;
};

struct Hal {
    HalI2c i2c;
    HalSpi spi;
    HalUart uart;
    HalGpio gpio;
    HalAdc adc;
    HalClock clock;
};

inline uint32_t hal_millis(const Hal& h) {
    return h.clock.millis ? h.clock.millis(h.clock.ctx) : 0;
}

inline void hal_delay_us(const Hal& h, uint32_t us) {
    if (h.clock.delay_us) {
        h.clock.delay_us(h.clock.ctx, us);
    }
}

inline bool i2c_write(const Hal& h, uint8_t addr, const uint8_t* data, size_t len) {
    return h.i2c.write && h.i2c.write(h.i2c.ctx, addr, data, len);
}

inline bool i2c_read(const Hal& h, uint8_t addr, uint8_t* data, size_t len) {
    return h.i2c.read && h.i2c.read(h.i2c.ctx, addr, data, len);
}

inline bool i2c_write_read(const Hal& h, uint8_t addr, const uint8_t* w, size_t wl, uint8_t* r,
                           size_t rl) {
    if (h.i2c.write_read) {
        return h.i2c.write_read(h.i2c.ctx, addr, w, wl, r, rl);
    }
    return i2c_write(h, addr, w, wl) && i2c_read(h, addr, r, rl);
}

// Filled by firmware/src/hal/esp32_hal.cpp on device builds.
bool hal_init_esp32(Hal* out);

} // namespace habitat
