#include "harness.hpp"

#include "habitat/frame.h"

#include <cstring>

void test_frame() {
    const char payload[] = "node=external_weather ts=1";
    habitat::Frame fr{};
    fr.version = 1;
    fr.node = habitat::NodeId::ExternalWeather;
    fr.type = habitat::MsgType::Telemetry;
    fr.seq = 7;
    fr.payload_len = static_cast<uint16_t>(std::strlen(payload));
    fr.payload = reinterpret_cast<const uint8_t*>(payload);

    uint8_t wire[256];
    const size_t n = habitat::encode_frame(wire, sizeof(wire), fr);
    EXPECT_TRUE(n > 10);
    EXPECT_EQ(wire[0], 0xA5);
    EXPECT_EQ(wire[1], 0x5A);
    EXPECT_EQ(wire[3], 1);
    EXPECT_EQ(wire[5], 7);

    habitat::FrameDecoder dec;
    habitat::Frame out{};
    uint8_t pl[200];
    bool got = false;
    for (size_t i = 0; i < n; ++i) {
        if (dec.feed(wire[i], &out, pl, sizeof(pl))) {
            got = true;
        }
    }
    EXPECT_TRUE(got);
    EXPECT_EQ(static_cast<int>(out.node), 1);
    EXPECT_EQ(out.seq, 7);
    EXPECT_EQ(out.payload_len, fr.payload_len);
    EXPECT_EQ(std::memcmp(pl, payload, fr.payload_len), 0);

    // Corrupt CRC: decoder must reject.
    wire[n - 1] ^= 0xFF;
    dec.reset();
    got = false;
    for (size_t i = 0; i < n; ++i) {
        if (dec.feed(wire[i], &out, pl, sizeof(pl))) {
            got = true;
        }
    }
    EXPECT_TRUE(!got);

    // Noise before SOF is ignored.
    dec.reset();
    const uint8_t noise[] = {0x00, 0x11, 0xA5, 0x00};
    for (uint8_t b : noise) {
        EXPECT_TRUE(!dec.feed(b, &out, pl, sizeof(pl)));
    }

    EXPECT_EQ(habitat::encode_frame(wire, 4, fr), 0U);
}
