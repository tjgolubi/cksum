#pragma once
#include <bit>
#include <cstdint>
#include <cstddef>

extern const std::uint32_t CrcTab2[8][256];
extern const std::uint32_t CrcTab[8][256];

inline std::uint32_t CrcUpdate2(std::uint32_t crc, std::byte b) noexcept {
  return (crc << 8) ^ CrcTab2[0][std::to_integer<int>(std::byte(crc >> 24) ^ b)];
}

inline std::uint32_t
CrcUpdate2(std::uint32_t crc, const void* buf, std::size_t len) noexcept {
  auto cp = reinterpret_cast<const std::byte*>(buf);
  while (len--)
    crc = CrcUpdate2(crc, *cp++);
  return crc;
}

inline std::uint32_t CrcUpdate(std::uint32_t crc, std::byte b) noexcept {
  return (crc >> 8) ^ CrcTab[0][std::to_integer<int>(std::byte(crc) ^ b)];
}

inline std::uint32_t
CrcUpdate(std::uint32_t crc, const void* buf, std::size_t len) noexcept {
  auto cp = reinterpret_cast<const std::byte*>(buf);
  while (len--)
    crc = CrcUpdate(crc, *cp++);
  return crc;
}
