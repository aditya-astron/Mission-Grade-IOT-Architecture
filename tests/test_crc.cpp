#include "harness.hpp"

#include "habitat/crc.h"

void test_crc() {
    const uint8_t msg[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    EXPECT_EQ(habitat::crc16_ccitt(msg, sizeof(msg)), 0x29B1);

    const uint8_t empty[] = {0};
    EXPECT_EQ(habitat::crc16_ccitt(empty, 0), 0xFFFF);

    const uint8_t beef[2] = {0xBE, 0xEF};
    EXPECT_EQ(habitat::crc8_sensirion(beef, 2), 0x92);

    const uint8_t co2[2] = {0x03, 0x20};
    EXPECT_EQ(habitat::crc8_sensirion(co2, 2), 0x2A);
}
