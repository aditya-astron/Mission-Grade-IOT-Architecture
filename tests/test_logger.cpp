#include "harness.hpp"

#include "habitat/logger.h"

#include <cstring>
#include <string>

static std::string g_last;

static void sink(const char* line, size_t len, void*) {
    g_last.assign(line, len);
}

void test_logger() {
    habitat::LogRecord rec{habitat::LogLevel::Info, 42, "sys", "boot", "external_weather"};
    char buf[128];
    EXPECT_TRUE(habitat::format_log(buf, sizeof(buf), rec) > 0);
    EXPECT_TRUE(std::strstr(buf, "INFO") != nullptr);
    EXPECT_TRUE(std::strstr(buf, "comp=sys") != nullptr);
    EXPECT_TRUE(std::strstr(buf, "event=boot") != nullptr);

    g_last.clear();
    habitat::logger_set_sink(sink, nullptr);
    habitat::logger_set_min_level(habitat::LogLevel::Warn);
    habitat::log_write(habitat::LogLevel::Info, 1, "sys", "skip", "-");
    EXPECT_TRUE(g_last.empty());
    habitat::log_write(habitat::LogLevel::Error, 2, "sys", "fail", "i2c");
    EXPECT_TRUE(g_last.find("ERROR") != std::string::npos);
    EXPECT_TRUE(g_last.find("event=fail") != std::string::npos);
    habitat::logger_set_min_level(habitat::LogLevel::Debug);
}
