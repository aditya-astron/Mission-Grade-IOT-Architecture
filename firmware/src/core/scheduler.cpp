#include "habitat/scheduler.h"

namespace habitat {

bool Scheduler::add(TaskFn fn, uint32_t period_ms, void* user, bool run_immediately) {
    if (fn == nullptr || period_ms == 0 || count_ >= (sizeof(tasks_) / sizeof(tasks_[0]))) {
        return false;
    }
    tasks_[count_++] = Task{fn, user, period_ms, 0, run_immediately};
    return true;
}

void Scheduler::tick(uint32_t now_ms) {
    for (size_t i = 0; i < count_; ++i) {
        Task& t = tasks_[i];
        if (!t.armed) {
            t.next_ms = now_ms + t.period_ms;
            t.armed = true;
            continue;
        }
        // Unsigned wrap-safe: due when (now - next) is small (now reached next).
        if (static_cast<int32_t>(now_ms - t.next_ms) >= 0) {
            t.next_ms = now_ms + t.period_ms;
            t.fn(now_ms, t.user);
        }
    }
}

void Scheduler::reset() {
    count_ = 0;
}

} // namespace habitat
