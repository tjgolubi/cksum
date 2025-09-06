#include "CrcUpdate.hpp"

#include "cksum.hpp"

#include <vector>
#include <bit>

#include <x86intrin.h>

using uint128_t = unsigned __int128;

uint128_t do_cksum_pclmula(std::uint32_t crc, const uint128_t* buf, size_t num)
  noexcept
{

  const __m128i single_mult_constant = _mm_set_epi64x(0xc5b9cd4c, 0xe8a45605);
  const __m128i four_mult_constant   = _mm_set_epi64x(0x8833794c, 0xe6228b11);

  const __m128i shuffle_constant = _mm_set_epi8(
                          0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);

  const __m128i* datap = (const __m128i*) buf;

  __m128i data0;
  __m128i temp0;

  data0  = datap[0]; data0  = _mm_shuffle_epi8(data0, shuffle_constant);
  temp0 = _mm_set_epi32(crc, 0, 0, 0);
  data0 ^= temp0;

  if (num >= 8) {
    __m128i data1;
    __m128i temp1;
    __m128i data2;
    __m128i temp2;
    __m128i data3;
    __m128i temp3;

    data1 = datap[1]; data1 = _mm_shuffle_epi8(data1, shuffle_constant);
    data2 = datap[2]; data2 = _mm_shuffle_epi8(data2, shuffle_constant);
    data3 = datap[3]; data3 = _mm_shuffle_epi8(data3, shuffle_constant);

    for ( ; num >= 8; num -= 4) {
      datap += 4;

      temp0 = _mm_clmulepi64_si128(data0, four_mult_constant, 0x00);
      data0 = _mm_clmulepi64_si128(data0, four_mult_constant, 0x11);
      temp1 = _mm_clmulepi64_si128(data1, four_mult_constant, 0x00);
      data1 = _mm_clmulepi64_si128(data1, four_mult_constant, 0x11);
      temp2 = _mm_clmulepi64_si128(data2, four_mult_constant, 0x00);
      data2 = _mm_clmulepi64_si128(data2, four_mult_constant, 0x11);
      temp3 = _mm_clmulepi64_si128(data3, four_mult_constant, 0x00);
      data3 = _mm_clmulepi64_si128(data3, four_mult_constant, 0x11);

      data0 ^= temp0;
      temp0 = datap[0]; temp0 = _mm_shuffle_epi8(temp0, shuffle_constant);
      data0 ^= temp0;

      data1 ^= temp1;
      temp1 = datap[1]; temp1 = _mm_shuffle_epi8(temp1, shuffle_constant);
      data1 ^= temp1;

      data2 ^= temp2;
      temp2 = datap[2]; temp2 = _mm_shuffle_epi8(temp2, shuffle_constant);
      data2 ^= temp2;

      data3 ^= temp3;
      temp3 = datap[3]; temp3 = _mm_shuffle_epi8(temp3, shuffle_constant);
      data3 ^= temp3;
    }

    temp0 = _mm_clmulepi64_si128(data0, single_mult_constant, 0x00);
    data0 = _mm_clmulepi64_si128(data0, single_mult_constant, 0x11);
    data0 ^= temp0;
    data0 ^= data1;

    temp0 = _mm_clmulepi64_si128(data0, single_mult_constant, 0x00);
    data0 = _mm_clmulepi64_si128(data0, single_mult_constant, 0x11);
    data0 ^= temp0;
    data0 ^= data2;

    temp0 = _mm_clmulepi64_si128(data0, single_mult_constant, 0x00);
    data0 = _mm_clmulepi64_si128(data0, single_mult_constant, 0x11);
    data0 ^= temp0;
    data0 ^= data3;

    num   -= 3;
    datap += 3;
  }

  for ( ; num >= 2; --num) {
    temp0 = _mm_clmulepi64_si128(data0, single_mult_constant, 0x00);
    data0 = _mm_clmulepi64_si128(data0, single_mult_constant, 0x11);
    data0 ^= temp0;
    temp0 = *++datap; temp0 = _mm_shuffle_epi8(temp0, shuffle_constant);
    data0 ^= temp0;
  }

  data0 = _mm_shuffle_epi8(data0, shuffle_constant);
  return (uint128_t) data0;
} // do_cksum_pclmula

std::uint32_t
cksum_pclmula(std::uint32_t crc, const void* buf, std::size_t size) noexcept {
  auto n = size / sizeof(uint128_t);
  auto r = size % sizeof(uint128_t);
  if (n < 2) {
    crc = std::byteswap(crc);
    crc = CrcUpdate(crc, buf, size);
    return std::byteswap(crc);
  }
  auto p = reinterpret_cast<const uint128_t*>(buf);
  auto u = do_cksum_pclmula(crc, p, n);
  crc = CrcUpdate(0, &u, sizeof(u));
  crc = CrcUpdate(crc, p+n,r);
  return std::byteswap(crc);
} // cksum_pclmula
