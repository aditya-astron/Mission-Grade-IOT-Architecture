#ifdef ARDUINO

#include "habitat/alert.h"
#include "habitat/algo.h"
#include "habitat/config.h"
#include "habitat/drivers.h"
#include "habitat/frame.h"
#include "habitat/hal.h"
#include "habitat/logger.h"
#include "habitat/pins.h"
#include "habitat/scheduler.h"
#include "habitat/telemetry.h"

#include <Arduino.h>

namespace {

using habitat::AlertEvent;
using habitat::AnalogSensor;
using habitat::Bmp280Driver;
using habitat::Bmp280Reading;
using habitat::Bno055Driver;
using habitat::Channel;
using habitat::Dht11Driver;
using habitat::Dht11Reading;
using habitat::Frame;
using habitat::GeigerCounter;
using habitat::GeigerDose;
using habitat::Hal;
using habitat::LogLevel;
using habitat::MsgType;
using habitat::NodeId;
using habitat::Sample;
using habitat::Scd4xDriver;
using habitat::Scd4xReading;
using habitat::Scheduler;
using habitat::Severity;
using habitat::TelemetryPacket;
using habitat::ThresholdEngine;

Hal g_hal;
Scheduler g_sched;
ThresholdEngine g_alerts;
uint8_t g_seq = 0;
bool g_led = false;

Bmp280Driver g_bmp(&g_hal, habitat::config::kBmp280AddrPrimary);
Bmp280Driver g_bmp_alt(&g_hal, habitat::config::kBmp280AddrSecondary);
Bmp280Driver* g_bmp_active = nullptr;
Dht11Driver g_dht(&g_hal, habitat::pins::kDht11Data);
AnalogSensor g_anem(&g_hal, habitat::pins::kAnemometerAdc);
AnalogSensor g_mq9(&g_hal, habitat::pins::kMq9Adc);
AnalogSensor g_mq135(&g_hal, habitat::pins::kMq135Adc);
GeigerCounter g_geiger(&g_hal, habitat::pins::kGeigerPulse);
Bno055Driver g_bno(&g_hal, habitat::config::kBno055Addr);
Scd4xDriver g_scd(&g_hal, habitat::config::kScd4xAddr);

void IRAM_ATTR geiger_isr() {
    g_geiger.on_pulse();
}

void uart_sink(const char* line, size_t len, void*) {
    Serial.write(reinterpret_cast<const uint8_t*>(line), len);
    Serial.write('\n');
}

void emit_line(const char* line, size_t n) {
    if (n == 0) {
        return;
    }
    Serial.write(reinterpret_cast<const uint8_t*>(line), n);
    Serial.write('\n');
}

void emit_frame(MsgType type, const char* payload, size_t payload_len) {
    uint8_t buf[habitat::kFrameMaxBytes];
    Frame fr{};
    fr.version = habitat::config::kProtocolVersion;
    fr.node = habitat::config::kNode;
    fr.type = type;
    fr.seq = g_seq++;
    fr.payload_len = static_cast<uint16_t>(payload_len);
    fr.payload = reinterpret_cast<const uint8_t*>(payload);
    const size_t n = habitat::encode_frame(buf, sizeof(buf), fr);
    if (n > 0) {
        Serial.write(buf, n);
    }
}

void consider(ThresholdEngine& eng, const Sample& s) {
    AlertEvent ev{};
    if (!eng.evaluate(s, &ev)) {
        return;
    }
    char line[160];
    const size_t n = habitat::format_alert(line, sizeof(line), habitat::config::kNode, ev);
    emit_line(line, n);
    emit_frame(MsgType::Alert, line, n);
    const bool alarm = ev.severity == Severity::Alarm;
    digitalWrite(habitat::pins::kAlertBuzzer, alarm ? HIGH : LOW);
    habitat::log_write(alarm ? LogLevel::Error : LogLevel::Warn, s.timestamp_ms, "alert",
                       habitat::channel_name(ev.channel), habitat::severity_name(ev.severity));
}

Sample make_sample(Channel ch, float value, uint32_t now, bool valid) {
    Sample s{};
    s.channel = ch;
    s.value = value;
    s.timestamp_ms = now;
    s.valid = valid;
    return s;
}

void add_if(TelemetryPacket* pkt, ThresholdEngine& eng, const Sample& s) {
    habitat::packet_add(pkt, s);
    consider(eng, s);
}

void sample_weather(TelemetryPacket* pkt, uint32_t now) {
    Dht11Reading dht{};
    const bool dht_ok = g_dht.read(&dht);
    add_if(pkt, g_alerts, make_sample(Channel::TempC, dht.temp_c, now, dht_ok));
    add_if(pkt, g_alerts, make_sample(Channel::HumidityRh, dht.humidity_rh, now, dht_ok));
    if (g_bmp_active != nullptr) {
        Bmp280Reading bmp{};
        const bool ok = g_bmp_active->read(&bmp);
        add_if(pkt, g_alerts, make_sample(Channel::PressureHpa, bmp.pressure_pa / 100.0f, now, ok));
    }
}

void sample_radiation(TelemetryPacket* pkt, uint32_t now) {
    const float mps = habitat::anemometer_mps_from_adc(
        g_anem.read_raw(), habitat::config::kAdcMaxRaw, habitat::config::kAdcFullScaleV);
    add_if(pkt, g_alerts, make_sample(Channel::WindMps, mps, now, true));
    const GeigerDose dose =
        g_geiger.sample(now, habitat::config::kGeigerWindowMs, habitat::config::kGeigerUsvPerCpm);
    add_if(pkt, g_alerts, make_sample(Channel::Cpm, dose.cpm, now, true));
    add_if(pkt, g_alerts, make_sample(Channel::DoseRateUSvh, dose.usv_h, now, true));
}

void sample_air(TelemetryPacket* pkt, uint32_t now) {
    const int mq9 = g_mq9.read_raw();
    const int mq135 = g_mq135.read_raw();
    add_if(pkt, g_alerts, make_sample(Channel::Mq9Raw, static_cast<float>(mq9), now, true));
    add_if(pkt, g_alerts, make_sample(Channel::Mq135Raw, static_cast<float>(mq135), now, true));
    if (g_bmp_active != nullptr) {
        Bmp280Reading bmp{};
        const bool ok = g_bmp_active->read(&bmp);
        add_if(pkt, g_alerts, make_sample(Channel::TempC, bmp.temp_c, now, ok));
        add_if(pkt, g_alerts, make_sample(Channel::PressureHpa, bmp.pressure_pa / 100.0f, now, ok));
    }
}

void sample_imu(TelemetryPacket* pkt, uint32_t now) {
    float x = 0, y = 0, z = 0;
    const bool ok = g_bno.read_accel(&x, &y, &z);
    add_if(pkt, g_alerts,
           make_sample(Channel::AccelMagMs2, habitat::accel_magnitude(x, y, z), now, ok));
}

void sample_co2(TelemetryPacket* pkt, uint32_t now) {
    Scd4xReading scd{};
    const bool scd_ok = g_scd.read(&scd);
    add_if(pkt, g_alerts,
           make_sample(Channel::Co2Ppm, static_cast<float>(scd.co2_ppm), now, scd_ok));
    add_if(pkt, g_alerts, make_sample(Channel::TempC, scd.temp_c, now, scd_ok));
    add_if(pkt, g_alerts, make_sample(Channel::HumidityRh, scd.humidity_rh, now, scd_ok));
    Dht11Reading dht{};
    const bool dht_ok = g_dht.read(&dht);
    if (!scd_ok && dht_ok) {
        add_if(pkt, g_alerts, make_sample(Channel::TempC, dht.temp_c, now, true));
        add_if(pkt, g_alerts, make_sample(Channel::HumidityRh, dht.humidity_rh, now, true));
    }
}

void sample_task(uint32_t now, void*) {
    TelemetryPacket pkt{};
    habitat::packet_clear(&pkt, habitat::config::kNode, now, g_seq);
    switch (habitat::config::kNode) {
    case NodeId::ExternalRadiation:
        sample_radiation(&pkt, now);
        break;
    case NodeId::InternalAir:
        sample_air(&pkt, now);
        break;
    case NodeId::InternalImu:
        sample_imu(&pkt, now);
        break;
    case NodeId::InternalCo2:
        sample_co2(&pkt, now);
        break;
    default:
        sample_weather(&pkt, now);
        break;
    }
    pkt.worst = Severity::Ok;
    size_t n = 0;
    const auto* defs = ThresholdEngine::defaults(&n);
    (void)defs;
    // worst is the highest latched channel severity
    for (size_t i = 0; i < pkt.sample_count; ++i) {
        const Severity s = g_alerts.severity(pkt.samples[i].channel);
        if (static_cast<uint8_t>(s) > static_cast<uint8_t>(pkt.worst)) {
            pkt.worst = s;
        }
    }
    char line[240];
    const size_t len = habitat::format_telemetry(line, sizeof(line), pkt);
    emit_line(line, len);
    emit_frame(MsgType::Telemetry, line, len);
    g_led = !g_led;
    digitalWrite(habitat::pins::kActivityLed, g_led ? HIGH : LOW);
}

void heartbeat_task(uint32_t now, void*) {
    habitat::log_write(LogLevel::Info, now, "sys", "heartbeat",
                       habitat::node_name(habitat::config::kNode));
}

bool init_bmp() {
    if (g_bmp.begin()) {
        g_bmp_active = &g_bmp;
        return true;
    }
    if (g_bmp_alt.begin()) {
        g_bmp_active = &g_bmp_alt;
        return true;
    }
    return false;
}

} // namespace

void setup() {
    Serial.begin(habitat::config::kUartBaud);
    delay(200);
    habitat::hal_init_esp32(&g_hal);
    habitat::logger_set_sink(uart_sink, nullptr);
    habitat::logger_set_min_level(LogLevel::Info);

    pinMode(habitat::pins::kActivityLed, OUTPUT);
    pinMode(habitat::pins::kAlertBuzzer, OUTPUT);
    digitalWrite(habitat::pins::kAlertBuzzer, LOW);

    size_t n = 0;
    const auto* defs = ThresholdEngine::defaults(&n);
    for (size_t i = 0; i < n; ++i) {
        g_alerts.add(defs[i]);
    }

    bool ok = true;
    switch (habitat::config::kNode) {
    case NodeId::ExternalWeather:
        ok = init_bmp();
        break;
    case NodeId::ExternalRadiation:
        ok = g_geiger.begin();
        if (g_hal.gpio.attach_isr) {
            g_hal.gpio.attach_isr(g_hal.gpio.ctx, habitat::pins::kGeigerPulse, geiger_isr,
                                  habitat::Edge::Rising);
        }
        break;
    case NodeId::InternalAir:
        ok = init_bmp();
        break;
    case NodeId::InternalImu:
        ok = g_bno.begin();
        break;
    case NodeId::InternalCo2:
        ok = g_scd.begin();
        break;
    default:
        break;
    }

    habitat::log_write(ok ? LogLevel::Info : LogLevel::Error, millis(), "sys", "boot",
                       habitat::node_name(habitat::config::kNode));

    g_sched.add(sample_task, habitat::config::kSamplePeriodMs, nullptr, true);
    g_sched.add(heartbeat_task, habitat::config::kHeartbeatPeriodMs, nullptr, false);
}

void loop() {
    g_sched.tick(millis());
}

#endif
