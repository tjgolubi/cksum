#include "CrcUpdate.hpp"
#include "Reflect.hpp"
#include "cksum.hpp"
#include "Int.hpp"

#include <concepts>
#include <type_traits>
#include <bit>

#include <x86intrin.h>

using uint128_t = unsigned __int128;

using U128 = tjg::Int<uint128_t, std::endian::big>;

template<std::integral T, std::endian E>
constexpr tjg::Int<T, ~E> Reflect(tjg::Int<T, E> x) noexcept
  { return tjg::Int<T, ~E>::Raw(tjg::IntMath::Reflect(x.raw())); }

#undef TJG_REFLECT

#ifdef TJG_REFLECT

constexpr __m128i TjgMulDiag(__m128i x, __m128i y) noexcept {
  auto t = _mm_clmulepi64_si128(x, y, 0x00);
       x = _mm_clmulepi64_si128(x, y, 0x11);
  return (x ^ t) << 1;
} // TjgMulDiag

#else

constexpr __m128i TjgMulDiag(__m128i x, __m128i y) noexcept {
  auto t = _mm_clmulepi64_si128(x, y, 0x00);
       x = _mm_clmulepi64_si128(x, y, 0x11);
  return x ^ t;
} // TjgMulDiag

#endif

U128 do_cksum_pclmul(std::uint32_t crc, const U128* buf, size_t num)
  noexcept
{
  using tjg::IntMath::Reflect;

  constexpr std::uint64_t SingleK_lo = 0xc5b9cd4c;
  constexpr std::uint64_t SingleK_hi = 0xe8a45605;
  constexpr std::uint64_t FourK_lo = 0x8833794c;
  constexpr std::uint64_t FourK_hi = 0xe6228b11;

#ifdef TJG_REFLECT
  const __m128i SingleK = _mm_set_epi64x(
                                    Reflect(SingleK_hi), Reflect(SingleK_lo));
  [[maybe_unused]]
  const __m128i FourK   = _mm_set_epi64x(Reflect(FourK_hi), Reflect(FourK_lo));
#else
  const __m128i SingleK = _mm_set_epi64x(SingleK_lo, SingleK_hi);
  [[maybe_unused]]
  const __m128i FourK   = _mm_set_epi64x(FourK_lo, FourK_hi);
#endif
  const __m128i ShuffleK = _mm_set_epi8(
                          0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);

#ifdef TJG_REFLECT
  auto Load = [&](U128 x) -> __m128i {
    auto y = ::Reflect(x);
    if constexpr (~U128::Endian == std::endian::native)
      return (__m128i) y.value();
    else
      return _mm_shuffle_epi8((__m128i) y.raw(), ShuffleK);
  };

  auto Unload = [&](__m128i x) -> U128 {
    uint128_t y;
    if constexpr (~U128::Endian == std::endian::native)
      y = (uint128_t) x;
    else
      y = (uint128_t) _mm_shuffle_epi8(x, ShuffleK);
    return U128::Raw(Reflect(y));
  };

#else
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
#endif

  __m128i data0 = Load(buf[0]);

#ifdef TJG_REFLECT
  data0 ^= (__m128i) uint128_t{Reflect(crc)};
#else
  data0 ^= (__m128i) (uint128_t{crc} << (128-32));
#endif

#if 0
  if (num >= 8) {
    __m128i data1 = Load(buf[1]);
    __m128i data2 = Load(buf[2]);
    __m128i data3 = Load(buf[3]);

    for ( ; num >= 8; num -= 4) {
      buf += 4;
      data0 = TjgMulDiag(data0, FourK) ^ Load(buf[0]);
      data1 = TjgMulDiag(data1, FourK) ^ Load(buf[1]);
      data2 = TjgMulDiag(data2, FourK) ^ Load(buf[2]);
      data3 = TjgMulDiag(data3, FourK) ^ Load(buf[3]);
    }

    data0 = TjgMulDiag(data0, SingleK) ^ data1;
    data0 = TjgMulDiag(data0, SingleK) ^ data2;
    data0 = TjgMulDiag(data0, SingleK) ^ data3;
    num -= 3;
    buf += 3;
  }
#endif
  for ( ; num >= 2; --num)
    data0 = TjgMulDiag(data0, SingleK) ^ Load(*++buf);
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
