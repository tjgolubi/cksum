#pragma once
#include <bit>
#include <cstdint>
#include <cstddef>

extern const std::uint32_t CrcTab[8][256];

using CrcType = std::uint32_t;

inline CrcType Lookup(std::byte x) noexcept
  { return std::byteswap(CrcTab[0][std::to_integer<int>(x)]); }

inline CrcType CrcUpdate(CrcType crc, std::byte b) noexcept {
  constexpr auto BitsPerByte = 8;
  constexpr auto LShift = 1 * BitsPerByte;
  constexpr auto RShift = (sizeof(CrcType) - 1) * BitsPerByte;
  return (crc << LShift) ^ Lookup(std::byte(crc >> RShift) ^ b);
}

inline
CrcType CrcUpdate(CrcType crc, const void* buf, std::size_t len) noexcept {
  auto cp = reinterpret_cast<const std::byte*>(buf);
  while (len--)
    crc = CrcUpdate(crc, *cp++);
  return crc;
}
