#include "habitat/alert.h"

#include "habitat/config.h"

namespace habitat {
namespace {

float band(float v, float frac) {
    return v * frac;
}

} // namespace

ThresholdEngine::Slot* ThresholdEngine::find(Channel channel) {
    for (size_t i = 0; i < count_; ++i) {
        if (slots_[i].spec.channel == channel) {
            return &slots_[i];
        }
    }
    return nullptr;
}

const ThresholdEngine::Slot* ThresholdEngine::find(Channel channel) const {
    for (size_t i = 0; i < count_; ++i) {
        if (slots_[i].spec.channel == channel) {
            return &slots_[i];
        }
    }
    return nullptr;
}

bool ThresholdEngine::add(const Threshold& t) {
    if (find(t.channel) != nullptr) {
        *find(t.channel) = Slot{t, Severity::Ok, Severity::Ok, 0};
        return true;
    }
    if (count_ >= (sizeof(slots_) / sizeof(slots_[0]))) {
        return false;
    }
    slots_[count_++] = Slot{t, Severity::Ok, Severity::Ok, 0};
    return true;
}

void ThresholdEngine::reset() {
    for (size_t i = 0; i < count_; ++i) {
        slots_[i].current = Severity::Ok;
        slots_[i].pending = Severity::Ok;
        slots_[i].hits = 0;
    }
}

Severity ThresholdEngine::severity(Channel channel) const {
    const Slot* s = find(channel);
    return s ? s->current : Severity::Ok;
}

Severity ThresholdEngine::classify(const Threshold& t, float value, Severity current) {
    const float h = config::kHysteresisFrac;

    auto above = [&](float limit, bool armed) {
        if (!armed) {
            return false;
        }
        if (current >= Severity::Warning && limit > 0) {
            return value > (limit - band(limit, h));
        }
        return value > limit;
    };
    auto below = [&](float limit, bool armed) {
        if (!armed) {
            return false;
        }
        if (current >= Severity::Warning && limit > 0) {
            return value < (limit + band(limit, h));
        }
        return value < limit;
    };

    if (above(t.alarm_high, t.has_high) || below(t.alarm_low, t.has_low)) {
        return Severity::Alarm;
    }
    if (above(t.warn_high, t.has_high) || below(t.warn_low, t.has_low)) {
        return Severity::Warning;
    }
    return Severity::Ok;
}

bool ThresholdEngine::evaluate(const Sample& sample, AlertEvent* event) {
    if (!sample.valid) {
        return false;
    }
    Slot* s = find(sample.channel);
    if (s == nullptr) {
        return false;
    }
    const Severity next = classify(s->spec, sample.value, s->current);
    if (next == s->pending) {
        if (s->hits < 255) {
            ++s->hits;
        }
    } else {
        s->pending = next;
        s->hits = 1;
    }
    if (s->pending == s->current || s->hits < config::kDebounceHits) {
        return false;
    }

    const Severity prev = s->current;
    s->current = s->pending;
    if (event == nullptr) {
        return true;
    }
    event->channel = sample.channel;
    event->severity = s->current;
    event->previous = prev;
    event->value = sample.value;
    event->timestamp_ms = sample.timestamp_ms;
    if (s->current == Severity::Alarm) {
        if (s->spec.has_high && sample.value > s->spec.alarm_high) {
            event->limit = s->spec.alarm_high;
        } else {
            event->limit = s->spec.alarm_low;
        }
    } else if (s->current == Severity::Warning) {
        if (s->spec.has_high && sample.value > s->spec.warn_high) {
            event->limit = s->spec.warn_high;
        } else {
            event->limit = s->spec.warn_low;
        }
    } else {
        event->limit = 0;
    }
    return true;
}

const Threshold* ThresholdEngine::defaults(size_t* count) {
    static const Threshold kDefaults[] = {
        {Channel::TempC, config::kTempWarnLowC, config::kTempWarnHighC, config::kTempAlarmLowC,
         config::kTempAlarmHighC, true, true},
        {Channel::HumidityRh, config::kRhWarnLow, config::kRhWarnHigh, config::kRhAlarmLow,
         config::kRhAlarmHigh, true, true},
        {Channel::PressureHpa, config::kPressWarnLowHpa, config::kPressWarnHighHpa,
         config::kPressAlarmLowHpa, config::kPressAlarmHighHpa, true, true},
        {Channel::WindMps, 0, config::kWindWarnMps, 0, config::kWindAlarmMps, false, true},
        {Channel::DoseRateUSvh, 0, config::kDoseWarnUsvh, 0, config::kDoseAlarmUsvh, false, true},
        {Channel::Co2Ppm, 0, config::kCo2WarnPpm, 0, config::kCo2AlarmPpm, false, true},
        {Channel::Mq9Raw, 0, static_cast<float>(config::kMq9LpgModerate), 0,
         static_cast<float>(config::kMq9LpgHigh), false, true},
        {Channel::Mq135Raw, 0, static_cast<float>(config::kMq135Warning), 0,
         static_cast<float>(config::kMq135Alarm), false, true},
        {Channel::AccelMagMs2, 0, config::kAccelWarnMs2, 0, config::kAccelAlarmMs2, false, true},
    };
    if (count != nullptr) {
        *count = sizeof(kDefaults) / sizeof(kDefaults[0]);
    }
    return kDefaults;
}

} // namespace habitat
