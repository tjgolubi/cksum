#include "CrcUpdate.hpp"
#include "Reflect.hpp"
#include "cksum.hpp"
#include "Int.hpp"

#include <concepts>
#include <type_traits>
#include <bit>
#include <cstdio>

#include <x86intrin.h>

using uint128_t = unsigned __int128;
using U128 = uint128_t;

#define TJG_REFLECT

#ifdef TJG_REFLECT

__m128i Reflect(__m128i x) {
  x[0] = tjg::IntMath::Reflect((std::uint64_t) x[0]);
  x[1] = tjg::IntMath::Reflect((std::uint64_t) x[1]);
  return x;
}

#if 1
constexpr __m128i TjgMulDiag(__m128i x, __m128i y) noexcept {
  auto z = x[0] ^ x[1];
  x = (__m128i) tjg::IntMath::Reflect((uint128_t) z);
  return x;
} // TjgMulDiag
#if 0
constexpr __m128i TjgMulDiag(__m128i x, __m128i y) noexcept {
  x = Reflect(x);
  y = Reflect(y);
  auto t = _mm_clmulepi64_si128(x, y, 0x00);
       x = _mm_clmulepi64_si128(x, y, 0x11);
  x ^= t;
  x = (__m128i) tjg::IntMath::Reflect((uint128_t) x);
  return x;
} // TjgMulDiag
#endif
#else
constexpr __m128i TjgMulDiag(__m128i x, __m128i y) noexcept {
  auto t = _mm_clmulepi64_si128(x, y, 0x00);
       x = _mm_clmulepi64_si128(x, y, 0x11);
  x ^= t;
  x <<= 1;
  return x;
} // TjgMulDiag
#endif

#else

constexpr __m128i TjgMulDiag(__m128i x, __m128i y) noexcept {
  auto t = _mm_clmulepi64_si128(x, y, 0x00);
       x = _mm_clmulepi64_si128(x, y, 0x11);
  return x ^ t;
} // TjgMulDiag

#endif

void Dump(__m128i r) {
  U128 x = (U128) r;
#ifdef TJG_REFLECT
  x = tjg::IntMath::Reflect(x);
  x = std::byteswap(x);
#endif
  auto lo = static_cast<std::uint64_t>(x);
  auto hi = static_cast<std::uint64_t>(x>>64);
  std::printf("%016llx%016llx\n", hi, lo);
} // Dump

U128 do_cksum_pclmul(std::uint32_t crc, const U128* buf, size_t num)
  noexcept
{
  using tjg::IntMath::Reflect;

//  constexpr std::uint64_t SingleK_hi = 0xc5b9cd4c;
//  constexpr std::uint64_t SingleK_lo = 0xe8a45605;
    constexpr std::uint64_t SingleK_lo = 1;
    constexpr std::uint64_t SingleK_hi = 1;

#ifdef TJG_REFLECT
    const __m128i SingleK = _mm_set_epi64x(
                                      Reflect(SingleK_lo), Reflect(SingleK_hi));
#else
  const __m128i SingleK = _mm_set_epi64x(SingleK_hi, SingleK_lo);
#endif
  [[maybe_unused]]
  const __m128i ShuffleK = _mm_set_epi8(
                          0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);

#ifdef TJG_REFLECT

  auto Load = [&](U128 x) -> __m128i {
    __m128i r = (__m128i) Reflect(x);
    // r = _mm_shuffle_epi8(r, ShuffleK);
    return r;
  };

  auto Unload = [&](__m128i x) -> U128 {
    // x = _mm_shuffle_epi8((__m128i) x, ShuffleK);
    return Reflect((U128) x);
  };

#else

  auto Load = [&](U128 x) -> __m128i
    { return _mm_shuffle_epi8((__m128i) x, ShuffleK); };

  auto Unload = [&](__m128i x) -> U128
    { return (U128) _mm_shuffle_epi8(x, ShuffleK); };

#endif

  __m128i data0 = Load(buf[0]);

#if 0
#ifdef TJG_REFLECT
  data0 ^= (__m128i) uint128_t{Reflect(crc)};
#else
  data0 ^= (__m128i) (uint128_t{crc} << (128-32));
#endif
#endif

  Dump(data0);
  for ( ; num >= 2; --num) {
    data0 = TjgMulDiag(data0, SingleK) ^ Load(*++buf);
    Dump(data0);
  }
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
