#include "cksum.hpp"

#include "CrcUpdate.hpp"
#include "CrcConsts.hpp"
#include "Int.hpp"

#include <concepts>
#include <type_traits>
#include <bit>

#include <x86intrin.h>

using uint128_t = unsigned __int128;

using U128 = tjg::Int<uint128_t, std::endian::big>;

constexpr __m128i ClMulDiag(__m128i x, __m128i y) noexcept {
  auto t = _mm_clmulepi64_si128(x, y, 0x00);
       x = _mm_clmulepi64_si128(x, y, 0x11);
  return x ^ t;
} // ClMulDiag

U128 do_cksum_pclmul(std::uint32_t crc, const U128* buf, size_t num)
  noexcept
{
  using C = tjg::crc::CrcConsts<32, 0x04c11db7>;

  const __m128i SingleK = _mm_set_epi64x(C::K128_hi, C::K128_lo);
  const __m128i FourK   = _mm_set_epi64x(C::K512_hi, C::K512_lo);
  const __m128i ShuffleK = _mm_set_epi8(
                          0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);

  auto Load = [&](U128 x) -> __m128i {
    if constexpr (U128::Endian == std::endian::native)
      return (__m128i) x.value();
    else
      return _mm_shuffle_epi8((__m128i) x.raw(), ShuffleK);
  };

  auto Unload = [&](__m128i x) -> U128 {
    if constexpr (U128::Endian == std::endian::native)
      return U128::Raw((uint128_t) x);
    else
      return U128::Raw((uint128_t) _mm_shuffle_epi8(x, ShuffleK));
  };

  __m128i data0 = Load(buf[0]);

  data0 ^= (__m128i) (uint128_t{crc} << (128-32));

  if (num >= 8) {
    __m128i data1 = Load(buf[1]);
    __m128i data2 = Load(buf[2]);
    __m128i data3 = Load(buf[3]);

    for ( ; num >= 8; num -= 4) {
      buf += 4;
      data0 = ClMulDiag(data0, FourK) ^ Load(buf[0]);
      data1 = ClMulDiag(data1, FourK) ^ Load(buf[1]);
      data2 = ClMulDiag(data2, FourK) ^ Load(buf[2]);
      data3 = ClMulDiag(data3, FourK) ^ Load(buf[3]);
    }

    data0 = ClMulDiag(data0, SingleK) ^ data1;
    data0 = ClMulDiag(data0, SingleK) ^ data2;
    data0 = ClMulDiag(data0, SingleK) ^ data3;
    num -= 3;
    buf += 3;
  }
  for ( ; num >= 2; --num)
    data0 = ClMulDiag(data0, SingleK) ^ Load(*++buf);
  return Unload(data0);
} // do_cksum_pclmul

std::uint32_t
cksum_pclmul(std::uint32_t crc, const void* buf, std::size_t size) noexcept {
  auto n = size / sizeof(U128);
  auto r = size % sizeof(U128);
  if (n < 2) {
    crc = std::byteswap(crc);
    crc = CrcUpdate(crc, buf, size);
    return std::byteswap(crc);
  }
  auto p = reinterpret_cast<const U128*>(buf);
  auto u = do_cksum_pclmul(crc, p, n);
  crc = CrcUpdate(0, &u, sizeof(u));
  crc = CrcUpdate(crc, p+n,r);
  return std::byteswap(crc);
} // cksum_pclmul
