CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Werror -O2 -I../firmware/include -I.
LDFLAGS ?=

CORE = \
	../firmware/src/core/crc.cpp \
	../firmware/src/core/frame.cpp \
	../firmware/src/core/alert.cpp \
	../firmware/src/core/telemetry.cpp \
	../firmware/src/core/logger.cpp \
	../firmware/src/core/scheduler.cpp \
	../firmware/src/algo/bmp280.cpp \
	../firmware/src/algo/sensors.cpp \
	../firmware/src/drivers/bmp280_driver.cpp \
	../firmware/src/drivers/bno055_driver.cpp \
	../firmware/src/drivers/scd4x_driver.cpp \
	../firmware/src/drivers/simple_sensors.cpp

TESTS = \
	test_main.cpp \
	test_crc.cpp \
	test_frame.cpp \
	test_alert.cpp \
	test_telemetry.cpp \
	test_algo.cpp \
	test_scheduler.cpp \
	test_logger.cpp \
	test_drivers.cpp

BIN = host_tests

.PHONY: all test clean
all: test

$(BIN): $(TESTS) $(CORE)
	$(CXX) $(CXXFLAGS) -o $@ $(TESTS) $(CORE) $(LDFLAGS)

test: $(BIN)
	./$(BIN)

clean:
	rm -f $(BIN)
