#pragma once

#include "habitat/types.h"

#include <cstddef>
#include <cstdint>

namespace habitat {

struct Threshold {
    Channel channel;
    float warn_low;
    float warn_high;
    float alarm_low;
    float alarm_high;
    bool has_low;
    bool has_high;
};

struct AlertEvent {
    Channel channel;
    Severity severity;
    Severity previous;
    float value;
    float limit;
    uint32_t timestamp_ms;
};

class ThresholdEngine {
  public:
    bool add(const Threshold& t);
    void reset();

    // Evaluate one sample. Returns true if severity changed after debounce.
    bool evaluate(const Sample& sample, AlertEvent* event);

    Severity severity(Channel channel) const;
    size_t count() const { return count_; }

    static const Threshold* defaults(size_t* count);

  private:
    struct Slot {
        Threshold spec;
        Severity current;
        Severity pending;
        uint8_t hits;
    };

    Slot slots_[12]{};
    size_t count_ = 0;
    Slot* find(Channel channel);
    const Slot* find(Channel channel) const;
    static Severity classify(const Threshold& t, float value, Severity current);
};

} // namespace habitat
