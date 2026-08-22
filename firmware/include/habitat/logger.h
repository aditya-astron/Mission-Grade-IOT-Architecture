#pragma once

#include "habitat/types.h"

#include <cstddef>
#include <cstdint>

namespace habitat {

struct LogRecord {
    LogLevel level;
    uint32_t timestamp_ms;
    const char* component;
    const char* event;
    const char* detail;
};

using LogSink = void (*)(const char* line, size_t len, void* user);

void logger_set_sink(LogSink sink, void* user);
void logger_set_min_level(LogLevel level);
size_t format_log(char* out, size_t cap, const LogRecord& rec);
void log_write(LogLevel level, uint32_t timestamp_ms, const char* component, const char* event,
               const char* detail);

} // namespace habitat
