#pragma once

#include "habitat/alert.h"
#include "habitat/types.h"

#include <cstddef>
#include <cstdint>

namespace habitat {

constexpr size_t kMaxSamples = 12;

struct TelemetryPacket {
    NodeId node;
    uint32_t timestamp_ms;
    uint8_t seq;
    Sample samples[kMaxSamples];
    size_t sample_count;
    Severity worst;
};

void packet_clear(TelemetryPacket* pkt, NodeId node, uint32_t timestamp_ms, uint8_t seq);
bool packet_add(TelemetryPacket* pkt, const Sample& sample);
Severity packet_worst(const TelemetryPacket& pkt);

// Compact key=value line, no heap. Example:
// node=external_weather ts=12000 seq=3 worst=ok temp_c=22.50 humidity_rh=41.00
size_t format_telemetry(char* out, size_t cap, const TelemetryPacket& pkt);
size_t format_alert(char* out, size_t cap, NodeId node, const AlertEvent& ev);

// Parse a telemetry line produced by format_telemetry. Used by host tests
// and a serial decoder. Returns false on malformed input.
bool parse_telemetry(const char* line, TelemetryPacket* pkt);

} // namespace habitat
