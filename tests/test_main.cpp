#include "harness.hpp"

#include <cstdio>

int main() {
    test_crc();
    test_frame();
    test_alert();
    test_telemetry();
    test_algo();
    test_scheduler();
    test_logger();
    test_drivers();

    std::printf("passed=%d failed=%d\n", stats().passed, stats().failed);
    return stats().failed == 0 ? 0 : 1;
}
