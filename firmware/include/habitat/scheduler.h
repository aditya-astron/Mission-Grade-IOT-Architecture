#pragma once

#include <cstddef>
#include <cstdint>

namespace habitat {

using TaskFn = void (*)(uint32_t now_ms, void* user);

class Scheduler {
  public:
    bool add(TaskFn fn, uint32_t period_ms, void* user = nullptr, bool run_immediately = true);
    void tick(uint32_t now_ms);
    size_t task_count() const { return count_; }
    void reset();

  private:
    struct Task {
        TaskFn fn;
        void* user;
        uint32_t period_ms;
        uint32_t next_ms;
        bool armed;
    };
    Task tasks_[8]{};
    size_t count_ = 0;
};

} // namespace habitat
