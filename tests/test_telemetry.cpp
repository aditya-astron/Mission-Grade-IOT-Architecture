#include "harness.hpp"

#include "habitat/telemetry.h"

#include <cstring>

void test_telemetry() {
    habitat::TelemetryPacket pkt{};
    habitat::packet_clear(&pkt, habitat::NodeId::ExternalWeather, 12000, 3);
    habitat::Sample s{};
    s.channel = habitat::Channel::TempC;
    s.value = 22.5f;
    s.timestamp_ms = 12000;
    s.valid = true;
    EXPECT_TRUE(habitat::packet_add(&pkt, s));
    s.channel = habitat::Channel::HumidityRh;
    s.value = 41.0f;
    EXPECT_TRUE(habitat::packet_add(&pkt, s));
    pkt.worst = habitat::Severity::Ok;

    char line[256];
    const size_t n = habitat::format_telemetry(line, sizeof(line), pkt);
    EXPECT_TRUE(n > 20);
    EXPECT_TRUE(std::strstr(line, "node=external_weather") != nullptr);
    EXPECT_TRUE(std::strstr(line, "temp_c=22.50") != nullptr);
    EXPECT_TRUE(std::strstr(line, "humidity_rh=41.00") != nullptr);

    habitat::TelemetryPacket back{};
    EXPECT_TRUE(habitat::parse_telemetry(line, &back));
    EXPECT_EQ(static_cast<int>(back.node), static_cast<int>(habitat::NodeId::ExternalWeather));
    EXPECT_EQ(back.timestamp_ms, 12000U);
    EXPECT_EQ(back.seq, 3);
    EXPECT_EQ(back.sample_count, 2U);
    EXPECT_NEAR(back.samples[0].value, 22.5, 0.01);
    EXPECT_NEAR(back.samples[1].value, 41.0, 0.01);

    EXPECT_TRUE(!habitat::parse_telemetry("not a packet", &back));
    EXPECT_TRUE(!habitat::parse_telemetry("foo=1", &back));

    habitat::AlertEvent ev{};
    ev.channel = habitat::Channel::Co2Ppm;
    ev.severity = habitat::Severity::Alarm;
    ev.previous = habitat::Severity::Warning;
    ev.value = 2400;
    ev.limit = 2000;
    ev.timestamp_ms = 99;
    char al[160];
    EXPECT_TRUE(habitat::format_alert(al, sizeof(al), habitat::NodeId::InternalCo2, ev) > 0);
    EXPECT_TRUE(std::strstr(al, "alert ") == al);
    EXPECT_TRUE(std::strstr(al, "ch=co2_ppm") != nullptr);
}
