# Hardware

Target MCU is an ESP32-WROOM (or equivalent DevKit) running at the Arduino
core's default 240 MHz. The pin map below matches the boards already wired
for analog-habitat bring-up. Change `firmware/include/habitat/pins.h` if a
later harness uses a different assignment.

## Node roles

| PlatformIO env        | Node id | Sensors                          | Buses        |
|-----------------------|---------|----------------------------------|--------------|
| `external_weather`    | 1       | DHT11, BMP280                    | 1-wire, I2C  |
| `external_radiation`  | 2       | analog anemometer, Geiger tube   | ADC, GPIO    |
| `internal_air`        | 3       | MQ-9, MQ-135, BMP280             | ADC, I2C     |
| `internal_imu`        | 4       | BNO055                           | I2C          |
| `internal_co2`        | 5       | SCD4x, DHT11                     | I2C, 1-wire  |

Two internal air modules can share the `internal_air` image. Set a distinct
UART identity in the field by flashing with a unique `HABITAT_NODE` only if
you add a second id; otherwise distinguish them at the host by serial port.

## Pin map (ESP32)

| Signal        | GPIO | Notes                                      |
|---------------|------|--------------------------------------------|
| I2C SDA       | 21   | BMP280, BNO055, SCD4x                      |
| I2C SCL       | 22   | 100 kHz default                            |
| DHT11 data    | 4    | 10 kΩ pull-up to 3.3 V                     |
| Anemometer    | 34   | ADC1, 0–3.3 V                              |
| Geiger pulse  | 13   | rising edge, INPUT_PULLUP                  |
| MQ-9 AO       | 32   | ADC1                                       |
| MQ-135 AO     | 33   | ADC1                                       |
| Activity LED  | 25   | toggled on each telemetry tick             |
| Alert buzzer  | 26   | high while any channel is in Alarm         |
| BMP280 CS     | 5    | reserved for the optional SPI path         |
| SPI SCK/MISO/MOSI | 18/19/23 | ESP32 VSPI, unused unless SPI BMP280  |

BMP280 addresses tried in order: `0x76`, then `0x77`. BNO055 default `0x28`.
SCD4x is `0x62`.

## Power

- 5 V USB or a regulated 5 V rail for the DevKit.
- 3.3 V sensor rail. MQ heaters draw a few hundred milliamps; do not power
  them from the ESP32 3.3 V pin.
- Geiger modules typically need their own high-voltage supply. Only the
  pulse output is connected to GPIO13.

## SPI

The hardware abstraction includes an SPI bus because BMP280 supports both
I2C and SPI. The default firmware path is I2C. The CS/SCK/MISO/MOSI pins
are reserved so a board that wires BMP280 on VSPI can be supported without
a pin-map rewrite.

## Bill of materials

See [bom.md](bom.md).
