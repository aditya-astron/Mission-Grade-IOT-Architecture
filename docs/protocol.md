# On-wire protocol

Firmware speaks UART at 115200 8N1. Every cycle it emits:

1. One human-readable telemetry line (and log / alert lines as needed).
2. The same payload wrapped in a binary frame with a CRC.

A host can use either. The binary frame is what you should persist; the
text line is for a serial monitor.

## Text telemetry

```
node=external_weather ts=12000 seq=3 worst=ok temp_c=22.50 humidity_rh=41.00
```

Keys are stable. Values are `%.2f` or `nan`. `worst` is the highest latched
threshold state among channels in that packet (`ok`, `warning`, `alarm`).

Alert line:

```
alert node=internal_co2 ch=co2_ppm sev=alarm prev=warning value=2400.00 limit=2000.00 ts=99
```

Log line:

```
INFO ts=200 comp=sys event=boot detail=external_weather
```

## Binary frame

| Offset | Size | Field                          |
|--------|------|--------------------------------|
| 0      | 1    | SOF0 `0xA5`                    |
| 1      | 1    | SOF1 `0x5A`                    |
| 2      | 1    | version (1)                    |
| 3      | 1    | node id                        |
| 4      | 1    | message type                   |
| 5      | 1    | sequence                       |
| 6      | 2    | payload length, big-endian     |
| 8      | N    | payload (UTF-8 text above)     |
| 8+N    | 2    | CRC-16/CCITT-FALSE, big-endian |

CRC covers bytes from `version` through the last payload byte (not the SOF
markers). Polynomial `0x1021`, init `0xFFFF`, no final XOR.

Message types: `1` telemetry, `2` alert, `3` log, `4` heartbeat.

Maximum payload is 200 bytes.

## I2C / SPI / UART map

| Bus  | Role                                                  |
|------|-------------------------------------------------------|
| I2C  | BMP280 (0x76/0x77), BNO055 (0x28), SCD4x (0x62)       |
| SPI  | Optional BMP280 path on VSPI (CS GPIO5)               |
| UART | Telemetry, alerts, structured logs to a host computer |

SCD4x words use Sensirion CRC-8 (poly `0x31`, init `0xFF`). A documented
check value is `CRC8(0xBE, 0xEF) = 0x92`.
