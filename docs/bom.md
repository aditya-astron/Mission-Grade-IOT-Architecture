# Bill of materials

Quantities assume one of each node type (five ESP32 boards). Buy extras for
harness rework.

| Item                         | Qty | Interface      | Notes                                      |
|------------------------------|-----|----------------|--------------------------------------------|
| ESP32-WROOM DevKit           | 5   | USB UART       | 3.3 V I/O                                  |
| DHT11                        | 2   | single-wire    | ±2 °C, 5–20 % RH; not a science-grade part |
| BMP280 breakout              | 3   | I2C (or SPI)   | 0x76 or 0x77                               |
| Analog anemometer            | 1   | 0–3.3 V ADC    | 32.4 m/s full scale on current boards      |
| Geiger tube + HV module      | 1   | pulse GPIO     | conversion 0.0057 µSv/h per CPM            |
| MQ-9 module                  | 2   | analog         | CO / LPG; raw ADC, not ppm                 |
| MQ-135 module                | 2   | analog         | VOCs / air quality; raw ADC, not ppm       |
| BNO055 breakout              | 1   | I2C 0x28       | accelerometer used for habitat shock       |
| SCD40 or SCD41               | 1   | I2C 0x62       | ambient CO2                                |
| 10 kΩ resistor               | 2   | —              | DHT11 pull-up                              |
| Piezo buzzer                 | 1   | GPIO           | radiation / air alarm                      |
| LED + 330 Ω                  | 5   | GPIO           | activity                                   |
| USB cables / 5 V supply      | 5   | —              | MQ heaters need a dedicated 5 V rail       |

Substitution is fine if the protocol stays the same (for example SCD41
instead of SCD40). Update `config.h` if the anemometer voltage span or
Geiger tube factor changes.
