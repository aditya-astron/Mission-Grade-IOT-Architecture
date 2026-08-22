#include "habitat/telemetry.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace habitat {
void packet_clear(TelemetryPacket* pkt, NodeId node, uint32_t timestamp_ms, uint8_t seq) {
    pkt->node = node;
    pkt->timestamp_ms = timestamp_ms;
    pkt->seq = seq;
    pkt->sample_count = 0;
    pkt->worst = Severity::Ok;
}

bool packet_add(TelemetryPacket* pkt, const Sample& sample) {
    if (pkt->sample_count >= kMaxSamples) {
        return false;
    }
    pkt->samples[pkt->sample_count++] = sample;
    return true;
}

Severity packet_worst(const TelemetryPacket& pkt) {
    return pkt.worst;
}

size_t format_telemetry(char* out, size_t cap, const TelemetryPacket& pkt) {
    if (out == nullptr || cap == 0) {
        return 0;
    }
    int n = std::snprintf(out, cap, "node=%s ts=%lu seq=%u worst=%s", node_name(pkt.node),
                          static_cast<unsigned long>(pkt.timestamp_ms), pkt.seq,
                          severity_name(pkt.worst));
    if (n < 0 || static_cast<size_t>(n) >= cap) {
        if (cap > 0) {
            out[0] = '\0';
        }
        return 0;
    }
    size_t used = static_cast<size_t>(n);
    for (size_t i = 0; i < pkt.sample_count; ++i) {
        const Sample& s = pkt.samples[i];
        char field[64];
        if (!s.valid || std::isnan(s.value)) {
            n = std::snprintf(field, sizeof(field), " %s=nan", channel_name(s.channel));
        } else {
            n = std::snprintf(field, sizeof(field), " %s=%.2f", channel_name(s.channel), s.value);
        }
        if (n < 0) {
            return 0;
        }
        if (used + static_cast<size_t>(n) + 1 >= cap) {
            return 0;
        }
        std::memcpy(out + used, field, static_cast<size_t>(n) + 1);
        used += static_cast<size_t>(n);
    }
    return used;
}

size_t format_alert(char* out, size_t cap, NodeId node, const AlertEvent& ev) {
    if (out == nullptr || cap == 0) {
        return 0;
    }
    const int n =
        std::snprintf(out, cap, "alert node=%s ch=%s sev=%s prev=%s value=%.2f limit=%.2f ts=%lu",
                      node_name(node), channel_name(ev.channel), severity_name(ev.severity),
                      severity_name(ev.previous), ev.value, ev.limit,
                      static_cast<unsigned long>(ev.timestamp_ms));
    if (n < 0 || static_cast<size_t>(n) >= cap) {
        if (cap > 0) {
            out[0] = '\0';
        }
        return 0;
    }
    return static_cast<size_t>(n);
}

bool parse_telemetry(const char* line, TelemetryPacket* pkt) {
    if (line == nullptr || pkt == nullptr) {
        return false;
    }
    packet_clear(pkt, NodeId::Unknown, 0, 0);
    const char* p = line;
    bool saw_node = false;
    while (*p) {
        while (*p == ' ') {
            ++p;
        }
        if (*p == '\0') {
            break;
        }
        const char* eq = std::strchr(p, '=');
        if (eq == nullptr) {
            return false;
        }
        char key[32];
        const size_t klen = static_cast<size_t>(eq - p);
        if (klen == 0 || klen >= sizeof(key)) {
            return false;
        }
        std::memcpy(key, p, klen);
        key[klen] = '\0';
        p = eq + 1;
        const char* end = std::strchr(p, ' ');
        size_t vlen = end ? static_cast<size_t>(end - p) : std::strlen(p);
        char val[48];
        if (vlen >= sizeof(val)) {
            return false;
        }
        std::memcpy(val, p, vlen);
        val[vlen] = '\0';
        p = end ? end : p + vlen;

        if (std::strcmp(key, "node") == 0) {
            if (std::strcmp(val, "external_weather") == 0) {
                pkt->node = NodeId::ExternalWeather;
            } else if (std::strcmp(val, "external_radiation") == 0) {
                pkt->node = NodeId::ExternalRadiation;
            } else if (std::strcmp(val, "internal_air") == 0) {
                pkt->node = NodeId::InternalAir;
            } else if (std::strcmp(val, "internal_imu") == 0) {
                pkt->node = NodeId::InternalImu;
            } else if (std::strcmp(val, "internal_co2") == 0) {
                pkt->node = NodeId::InternalCo2;
            } else {
                pkt->node = NodeId::Unknown;
            }
            saw_node = true;
        } else if (std::strcmp(key, "ts") == 0) {
            pkt->timestamp_ms = static_cast<uint32_t>(std::strtoul(val, nullptr, 10));
        } else if (std::strcmp(key, "seq") == 0) {
            pkt->seq = static_cast<uint8_t>(std::strtoul(val, nullptr, 10));
        } else if (std::strcmp(key, "worst") == 0) {
            if (std::strcmp(val, "alarm") == 0) {
                pkt->worst = Severity::Alarm;
            } else if (std::strcmp(val, "warning") == 0) {
                pkt->worst = Severity::Warning;
            } else {
                pkt->worst = Severity::Ok;
            }
        } else {
            Sample s{};
            s.valid = std::strcmp(val, "nan") != 0;
            s.value = s.valid ? static_cast<float>(std::atof(val)) : 0.0f;
            s.timestamp_ms = pkt->timestamp_ms;
            if (std::strcmp(key, "temp_c") == 0) {
                s.channel = Channel::TempC;
            } else if (std::strcmp(key, "humidity_rh") == 0) {
                s.channel = Channel::HumidityRh;
            } else if (std::strcmp(key, "pressure_hpa") == 0) {
                s.channel = Channel::PressureHpa;
            } else if (std::strcmp(key, "wind_mps") == 0) {
                s.channel = Channel::WindMps;
            } else if (std::strcmp(key, "dose_usv_h") == 0) {
                s.channel = Channel::DoseRateUSvh;
            } else if (std::strcmp(key, "co2_ppm") == 0) {
                s.channel = Channel::Co2Ppm;
            } else if (std::strcmp(key, "mq9_raw") == 0) {
                s.channel = Channel::Mq9Raw;
            } else if (std::strcmp(key, "mq135_raw") == 0) {
                s.channel = Channel::Mq135Raw;
            } else if (std::strcmp(key, "accel_mag_ms2") == 0) {
                s.channel = Channel::AccelMagMs2;
            } else if (std::strcmp(key, "cpm") == 0) {
                s.channel = Channel::Cpm;
            } else {
                return false;
            }
            if (!packet_add(pkt, s)) {
                return false;
            }
        }
    }
    return saw_node;
}

} // namespace habitat
