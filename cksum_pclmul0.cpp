#include "CrcUpdate.hpp"

#include "cksum.hpp"

#include <vector>
#include <bit>

#include <x86intrin.h>

using uint128_t = unsigned __int128;

using U128 = uint128_t;

U128 do_cksum_pclmul0(std::uint32_t crc, const U128* buf, size_t num) noexcept {
  const __m128i SingleK = _mm_set_epi64x(0xc5b9cd4c, 0xe8a45605);
  const __m128i FourK   = _mm_set_epi64x(0x8833794c, 0xe6228b11);

  const __m128i ShuffleK = _mm_set_epi8(
                          0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);

  const __m128i* datap = (const __m128i*) buf;

  __m128i data0;
  __m128i temp0;

  data0  = datap[0]; data0  = _mm_shuffle_epi8(data0, ShuffleK);
  temp0 = _mm_set_epi32(crc, 0, 0, 0);
  data0 ^= temp0;

  if (num >= 8) {
    __m128i data1;
    __m128i temp1;
    __m128i data2;
    __m128i temp2;
    __m128i data3;
    __m128i temp3;

    data1 = datap[1]; data1 = _mm_shuffle_epi8(data1, ShuffleK);
    data2 = datap[2]; data2 = _mm_shuffle_epi8(data2, ShuffleK);
    data3 = datap[3]; data3 = _mm_shuffle_epi8(data3, ShuffleK);

    for ( ; num >= 8; num -= 4) {
      datap += 4;

      temp0 = _mm_clmulepi64_si128(data0, FourK, 0x00);
      data0 = _mm_clmulepi64_si128(data0, FourK, 0x11);
      temp1 = _mm_clmulepi64_si128(data1, FourK, 0x00);
      data1 = _mm_clmulepi64_si128(data1, FourK, 0x11);
      temp2 = _mm_clmulepi64_si128(data2, FourK, 0x00);
      data2 = _mm_clmulepi64_si128(data2, FourK, 0x11);
      temp3 = _mm_clmulepi64_si128(data3, FourK, 0x00);
      data3 = _mm_clmulepi64_si128(data3, FourK, 0x11);

      data0 ^= temp0;
      temp0 = datap[0]; temp0 = _mm_shuffle_epi8(temp0, ShuffleK);
      data0 ^= temp0;

      data1 ^= temp1;
      temp1 = datap[1]; temp1 = _mm_shuffle_epi8(temp1, ShuffleK);
      data1 ^= temp1;

      data2 ^= temp2;
      temp2 = datap[2]; temp2 = _mm_shuffle_epi8(temp2, ShuffleK);
      data2 ^= temp2;

      data3 ^= temp3;
      temp3 = datap[3]; temp3 = _mm_shuffle_epi8(temp3, ShuffleK);
      data3 ^= temp3;
    }

    temp0 = _mm_clmulepi64_si128(data0, SingleK, 0x00);
    data0 = _mm_clmulepi64_si128(data0, SingleK, 0x11);
    data0 ^= temp0;
    data0 ^= data1;

    temp0 = _mm_clmulepi64_si128(data0, SingleK, 0x00);
    data0 = _mm_clmulepi64_si128(data0, SingleK, 0x11);
    data0 ^= temp0;
    data0 ^= data2;

    temp0 = _mm_clmulepi64_si128(data0, SingleK, 0x00);
    data0 = _mm_clmulepi64_si128(data0, SingleK, 0x11);
    data0 ^= temp0;
    data0 ^= data3;

    num   -= 3;
    datap += 3;
  }

  for ( ; num >= 2; --num) {
    temp0 = _mm_clmulepi64_si128(data0, SingleK, 0x00);
    data0 = _mm_clmulepi64_si128(data0, SingleK, 0x11);
    data0 ^= temp0;
    temp0 = *++datap; temp0 = _mm_shuffle_epi8(temp0, ShuffleK);
    data0 ^= temp0;
  }

  data0 = _mm_shuffle_epi8(data0, ShuffleK);
  return (U128) data0;
} // do_cksum_pclmul0

CrcType cksum_pclmul0(CrcType crc, const void* buf, std::size_t size) noexcept {
  auto n = size / sizeof(U128);
  auto r = size % sizeof(U128);
  if (n < 2)
    return CrcUpdate(crc, buf, size);
  auto p = reinterpret_cast<const U128*>(buf);
  auto u = do_cksum_pclmul0(crc, p, n);
  crc = CrcUpdate(CrcType{0}, &u, sizeof(u));
  crc = CrcUpdate(crc, p+n,r);
  return crc;
} // cksum_pclmul0
