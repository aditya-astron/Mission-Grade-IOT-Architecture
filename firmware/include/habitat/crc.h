#pragma once

#include <cstddef>
#include <cstdint>

namespace habitat {

// CRC-16/CCITT-FALSE: poly 0x1021, init 0xFFFF, refin/refout false, xorout 0x0000.
uint16_t crc16_ccitt(const uint8_t* data, size_t len, uint16_t seed = 0xFFFF);

// Sensirion CRC-8: poly 0x31, init 0xFF. Used by SCD4x.
uint8_t crc8_sensirion(const uint8_t* data, size_t len);

} // namespace habitat
