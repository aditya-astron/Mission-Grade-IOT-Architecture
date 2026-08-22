#include "habitat/logger.h"

#include <cstdio>
#include <cstring>

namespace habitat {
namespace {

LogSink g_sink = nullptr;
void* g_user = nullptr;
LogLevel g_min = LogLevel::Debug;

const char* level_name(LogLevel level) {
    switch (level) {
    case LogLevel::Debug:
        return "DEBUG";
    case LogLevel::Info:
        return "INFO";
    case LogLevel::Warn:
        return "WARN";
    case LogLevel::Error:
        return "ERROR";
    }
    return "INFO";
}

} // namespace

void logger_set_sink(LogSink sink, void* user) {
    g_sink = sink;
    g_user = user;
}

void logger_set_min_level(LogLevel level) {
    g_min = level;
}

size_t format_log(char* out, size_t cap, const LogRecord& rec) {
    if (out == nullptr || cap == 0) {
        return 0;
    }
    const int n = std::snprintf(out, cap, "%s ts=%lu comp=%s event=%s detail=%s",
                                level_name(rec.level), static_cast<unsigned long>(rec.timestamp_ms),
                                rec.component ? rec.component : "-", rec.event ? rec.event : "-",
                                rec.detail ? rec.detail : "-");
    if (n < 0 || static_cast<size_t>(n) >= cap) {
        if (cap > 0) {
            out[0] = '\0';
        }
        return 0;
    }
    return static_cast<size_t>(n);
}

void log_write(LogLevel level, uint32_t timestamp_ms, const char* component, const char* event,
               const char* detail) {
    if (static_cast<uint8_t>(level) < static_cast<uint8_t>(g_min)) {
        return;
    }
    char line[192];
    LogRecord rec{level, timestamp_ms, component, event, detail};
    const size_t n = format_log(line, sizeof(line), rec);
    if (n > 0 && g_sink != nullptr) {
        g_sink(line, n, g_user);
    }
}

} // namespace habitat
