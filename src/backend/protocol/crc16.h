/*
 * crc16.h — CRC-16/CCITT implementation
 *
 * Polynomial: 0x1021, Initial: 0xFFFF
 * Used for RPC frame integrity checking.
 */

#ifndef CRC16_H
#define CRC16_H

#include <cstdint>
#include <cstddef>

namespace rpc {

/**
 * Compute CRC-16/CCITT over a byte buffer.
 * @param data   Pointer to data
 * @param length Number of bytes
 * @param init   Initial CRC value (default 0xFFFF)
 * @return       16-bit CRC
 */
uint16_t crc16_ccitt(const uint8_t *data, size_t length, uint16_t init = 0xFFFF);

} // namespace rpc

#endif // CRC16_H
