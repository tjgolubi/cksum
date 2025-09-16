#pragma once
#include "Int.hpp"
#include <bit>
#include <cstdint>
#include <cstddef>

extern const std::uint32_t CrcTab[8][256];

using CrcType = tjg::BigUint32;

inline CrcType Lookup(std::byte x) noexcept
  { return CrcType::Raw(CrcTab[0][std::to_integer<int>(x)]); }

inline CrcType CrcUpdate(CrcType crc, std::byte b) noexcept
  { return crc.leftBytes(1) ^ Lookup(std::byte(crc.rightBytes(3)) ^ b); }

inline
CrcType CrcUpdate(CrcType crc, const void* buf, std::size_t len) noexcept {
  auto cp = reinterpret_cast<const std::byte*>(buf);
  while (len--)
    crc = CrcUpdate(crc, *cp++);
  return crc;
}
