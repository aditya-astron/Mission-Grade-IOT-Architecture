#include "harness.hpp"

#include "habitat/alert.h"
#include "habitat/config.h"

static habitat::Sample samp(habitat::Channel ch, float v, uint32_t ts) {
    return habitat::Sample{ch, v, ts, true};
}

void test_alert() {
    habitat::ThresholdEngine eng;
    habitat::Threshold t{};
    t.channel = habitat::Channel::Co2Ppm;
    t.warn_high = 1000;
    t.alarm_high = 2000;
    t.has_high = true;
    t.has_low = false;
    EXPECT_TRUE(eng.add(t));

    habitat::AlertEvent ev{};
    EXPECT_TRUE(!eng.evaluate(samp(habitat::Channel::Co2Ppm, 800, 1), &ev));
    EXPECT_EQ(static_cast<int>(eng.severity(habitat::Channel::Co2Ppm)), 0);

    // Debounce: first excursion is not yet a transition.
    EXPECT_TRUE(!eng.evaluate(samp(habitat::Channel::Co2Ppm, 1200, 2), &ev));
    EXPECT_TRUE(eng.evaluate(samp(habitat::Channel::Co2Ppm, 1200, 3), &ev));
    EXPECT_EQ(static_cast<int>(ev.severity), 1);
    EXPECT_EQ(static_cast<int>(ev.previous), 0);
    EXPECT_NEAR(ev.limit, 1000.0, 0.01);

    EXPECT_TRUE(!eng.evaluate(samp(habitat::Channel::Co2Ppm, 2500, 4), &ev));
    EXPECT_TRUE(eng.evaluate(samp(habitat::Channel::Co2Ppm, 2500, 5), &ev));
    EXPECT_EQ(static_cast<int>(ev.severity), 2);
    EXPECT_NEAR(ev.limit, 2000.0, 0.01);

    // Hysteresis: 5% below alarm still alarm if already there (2000 * 0.95 = 1900).
    EXPECT_TRUE(!eng.evaluate(samp(habitat::Channel::Co2Ppm, 1950, 6), &ev));
    EXPECT_TRUE(!eng.evaluate(samp(habitat::Channel::Co2Ppm, 1950, 7), &ev));
    EXPECT_EQ(static_cast<int>(eng.severity(habitat::Channel::Co2Ppm)), 2);

    // Drop well below warning to return to OK.
    EXPECT_TRUE(!eng.evaluate(samp(habitat::Channel::Co2Ppm, 100, 8), &ev));
    EXPECT_TRUE(eng.evaluate(samp(habitat::Channel::Co2Ppm, 100, 9), &ev));
    EXPECT_EQ(static_cast<int>(ev.severity), 0);

    habitat::Sample bad{habitat::Channel::Co2Ppm, 9000, 10, false};
    EXPECT_TRUE(!eng.evaluate(bad, &ev));

    size_t n = 0;
    const habitat::Threshold* defs = habitat::ThresholdEngine::defaults(&n);
    EXPECT_TRUE(n >= 8);
    EXPECT_EQ(static_cast<int>(defs[0].channel), static_cast<int>(habitat::Channel::TempC));
}
