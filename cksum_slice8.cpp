/* cksum -- calculate and print POSIX checksums and sizes of files
   Copyright (C) 1992-2025 Free Software Foundation, Inc. */

#include "cksum.hpp"

#include <bit>
#include <sys/types.h>

#if defined(__GNUC__) || defined(__clang__)
constexpr std::uint32_t ToBigEndian(std::uint32_t x) {
  if constexpr (std::endian::native == std::endian::big)
    return x;
  else
    return __builtin_bswap32(x);
} // ToBigEndian
#endif

uint_fast32_t cksum_slice8(uint_fast32_t crc,
                           void* buf, std::size_t* buflen) noexcept
{
  /* Process multiples of 8 bytes */
  auto datap = static_cast<const uint32_t*>(buf);
  auto num  = *buflen / 8;
  while (num >= 1) {
    std::uint32_t first  = *datap++;
    std::uint32_t second = *datap++;
    crc   ^= ToBigEndian(first);
    second = ToBigEndian(second);
    crc = CrcTab[7][(   crc >> 24) & 0xff]
        ^ CrcTab[6][(   crc >> 16) & 0xff]
        ^ CrcTab[5][(   crc >>  8) & 0xff]
        ^ CrcTab[4][(   crc >>  0) & 0xff]
        ^ CrcTab[3][(second >> 24) & 0xff]
        ^ CrcTab[2][(second >> 16) & 0xff]
        ^ CrcTab[1][(second >>  8) & 0xff]
        ^ CrcTab[0][(second >>  0) & 0xff];
    --num;
  }
  *buflen = *buflen % 8 + 8 * num;
  return crc;
} // cksum_slice8
