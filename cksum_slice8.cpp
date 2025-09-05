#include "CrcUpdate.hpp"
#include "cksum.hpp"
#include <cstdint>

std::uint32_t
do_cksum_slice8(std::uint32_t crc, const std::uint64_t* buf, std::size_t num)
  noexcept
{
  using std::uint8_t;
  auto p = reinterpret_cast<const std::uint32_t*>(buf);
  while (num--) {
    crc ^= *p++;
    auto hi = *p++;
    crc = CrcTab[7][uint8_t(crc >>  0)]
        ^ CrcTab[6][uint8_t(crc >>  8)]
        ^ CrcTab[5][uint8_t(crc >> 16)]
        ^ CrcTab[4][uint8_t(crc >> 24)]
        ^ CrcTab[3][uint8_t(hi  >>  0)]
        ^ CrcTab[2][uint8_t(hi  >>  8)]
        ^ CrcTab[1][uint8_t(hi  >> 16)]
        ^ CrcTab[0][uint8_t(hi  >> 24)];
  }
  return crc;
} // do_cksum_slice8

std::uint32_t cksum_slice8(std::uint32_t crc, const void* buf, std::size_t size)
  noexcept
{
  auto n = size / sizeof(std::uint64_t);
  auto r = size % sizeof(std::uint64_t);
  auto p = reinterpret_cast<const std::uint64_t*>(buf);
  crc = std::byteswap(crc);
  crc = do_cksum_slice8(crc, p, n);
  crc =  CrcUpdate(crc, p+n, r);
  return std::byteswap(crc);
} // cksum_slice8
