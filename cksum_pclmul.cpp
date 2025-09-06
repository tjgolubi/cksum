#include "CrcUpdate.hpp"
#include "cksum.hpp"

#include <bit>

#include <x86intrin.h>

using uint128_t = unsigned __int128;

constexpr __m128i ClMult(__m128i x, __m128i y) {
  auto t = _mm_clmulepi64_si128(x, y, 0x00);
       x = _mm_clmulepi64_si128(x, y, 0x11);
  return x ^ t;
} // ClMult

constexpr __m128i ByteSwap(__m128i x) {
  static const auto shuffle_constant = _mm_set_epi8(
                          0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
  return _mm_shuffle_epi8(x, shuffle_constant);
} // ByteSwap

constexpr __m128i ByteSwap(uint128_t x) { return ByteSwap((__m128i) x); }

uint128_t do_cksum_pclmul(std::uint32_t crc, const uint128_t* buf, size_t num)
  noexcept
{
  const __m128i single_mult_constant = _mm_set_epi64x(0xc5b9cd4c, 0xe8a45605);
  const __m128i four_mult_constant   = _mm_set_epi64x(0x8833794c, 0xe6228b11);

  __m128i data0 = ByteSwap(buf[0]);

  data0 ^= (__m128i) (uint128_t{crc} << (128-32));

  if (num >= 8) {
    __m128i data1 = ByteSwap(buf[1]);
    __m128i data2 = ByteSwap(buf[2]);
    __m128i data3 = ByteSwap(buf[3]);

    for ( ; num >= 8; num -= 4) {
      buf += 4;
      data0 = ClMult(data0, four_mult_constant) ^ ByteSwap(buf[0]);
      data1 = ClMult(data1, four_mult_constant) ^ ByteSwap(buf[1]);
      data2 = ClMult(data2, four_mult_constant) ^ ByteSwap(buf[2]);
      data3 = ClMult(data3, four_mult_constant) ^ ByteSwap(buf[3]);
    }

    data0 = ClMult(data0, single_mult_constant) ^ data1;
    data0 = ClMult(data0, single_mult_constant) ^ data2;
    data0 = ClMult(data0, single_mult_constant) ^ data3;
    num -= 3;
    buf += 3;
  }
  for ( ; num >= 2; --num)
    data0 = ClMult(data0, single_mult_constant) ^ ByteSwap(*++buf);
  return (uint128_t) ByteSwap(data0);
} // do_cksum_pclmul

std::uint32_t
cksum_pclmul(std::uint32_t crc, const void* buf, std::size_t size) noexcept {
  auto n = size / sizeof(uint128_t);
  auto r = size % sizeof(uint128_t);
  if (n < 2) {
    crc = std::byteswap(crc);
    crc = CrcUpdate(crc, buf, size);
    return std::byteswap(crc);
  }
  auto p = reinterpret_cast<const uint128_t*>(buf);
  auto u = do_cksum_pclmul(crc, p, n);
  crc = CrcUpdate(0, &u, sizeof(u));
  crc = CrcUpdate(crc, p+n,r);
  return std::byteswap(crc);
} // cksum_pclmul
