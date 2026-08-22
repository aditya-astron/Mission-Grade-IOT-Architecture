#pragma once

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

struct TestStats {
    int passed = 0;
    int failed = 0;
};

inline TestStats& stats() {
    static TestStats s;
    return s;
}

#define EXPECT_TRUE(cond)                                                                          \
    do {                                                                                           \
        if (cond) {                                                                                \
            ++stats().passed;                                                                      \
        } else {                                                                                   \
            ++stats().failed;                                                                      \
            std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);                   \
        }                                                                                          \
    } while (0)

#define EXPECT_EQ(a, b)                                                                            \
    do {                                                                                           \
        const auto _ea = (a);                                                                      \
        const auto _eb = (b);                                                                      \
        if (_ea == _eb) {                                                                          \
            ++stats().passed;                                                                      \
        } else {                                                                                   \
            ++stats().failed;                                                                      \
            std::fprintf(stderr, "FAIL %s:%d: %s == %s (%lld != %lld)\n", __FILE__, __LINE__, #a,  \
                         #b, static_cast<long long>(_ea), static_cast<long long>(_eb));            \
        }                                                                                          \
    } while (0)

#define EXPECT_NEAR(a, b, eps)                                                                     \
    do {                                                                                           \
        const double _ea = static_cast<double>(a);                                                 \
        const double _eb = static_cast<double>(b);                                                 \
        if (std::fabs(_ea - _eb) <= static_cast<double>(eps)) {                                    \
            ++stats().passed;                                                                      \
        } else {                                                                                   \
            ++stats().failed;                                                                      \
            std::fprintf(stderr, "FAIL %s:%d: %s ≈ %s (%g != %g)\n", __FILE__, __LINE__, #a, #b,   \
                         _ea, _eb);                                                                \
        }                                                                                          \
    } while (0)

#define EXPECT_STREQ(a, b)                                                                         \
    do {                                                                                           \
        if (std::strcmp((a), (b)) == 0) {                                                          \
            ++stats().passed;                                                                      \
        } else {                                                                                   \
            ++stats().failed;                                                                      \
            std::fprintf(stderr, "FAIL %s:%d: \"%s\" != \"%s\"\n", __FILE__, __LINE__, (a), (b));  \
        }                                                                                          \
    } while (0)

void test_crc();
void test_frame();
void test_alert();
void test_telemetry();
void test_algo();
void test_scheduler();
void test_logger();
void test_drivers();
