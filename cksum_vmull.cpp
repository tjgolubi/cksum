#include "CrcUpdate.hpp"
#include "Reflect.hpp"
#include "cksum.hpp"
#include "Neon.hpp"

#include "Int.hpp"

#include <bit>

constexpr uint128_t Uint128(std::uint64_t hi, std::uint64_t lo) noexcept
  { return (uint128_t{hi} << 64) | uint128_t{lo}; }

#if 0
namespace tjg::IntMath {

constexpr uint128_t Reflect(uint128_t x) noexcept {
  auto lo = static_cast<std::uint64_t>(x);
  auto hi = static_cast<std::uint64_t>(x >> 64);
  return Uint128(Reflect(lo), Reflect(hi));
}

} // tjg::IntMath
#endif

using U128 = uint128_t;

#undef TJG_REFLECT_MODE

#ifdef TJG_REFLECT_MODE
constexpr U128 Load(U128 x) noexcept { return tjg::IntMath::Reflect(x); }
constexpr NeonV<uint64x2_t> TjgMulDiag(NeonV<uint64x2_t> x, NeonV<poly64x2_t> y)
  noexcept
  { return ClMulDiag(x, y) << 1; }
#else
constexpr U128 Load(U128 x) noexcept { return std::byteswap(x); }
constexpr NeonV<uint64x2_t> TjgMulDiag(NeonV<uint64x2_t> x, NeonV<poly64x2_t> y)
  noexcept
  { return ClMulDiag(x, y); }
#endif

U128 do_cksum_vmull(std::uint32_t crc, const U128* buf, std::size_t num)
  noexcept
{
  using NeonVec  = NeonV<uint64x2_t>;
  using NeonPoly = NeonV<poly64x2_t>;

#if 1
  constexpr std::uint32_t SingleK_lo = 0xe8a45605;
  constexpr std::uint32_t SingleK_hi = 0xc5b9cd4c;
  constexpr std::uint32_t FourK_lo   = 0xe6228b11;
  constexpr std::uint32_t FourK_hi   = 0x8833794c;

#ifdef TJG_REFLECT_MODE
  using tjg::IntMath::Reflect;
  static const NeonPoly SingleK{Reflect(SingleK_hi), Reflect(SingleK_lo)};
  static const NeonPoly FourK  {Reflect(  FourK_hi), Reflect(  FourK_lo)};
#else
  static const NeonPoly SingleK{SingleK_lo, SingleK_hi};
  static const NeonPoly FourK  {  FourK_lo,   FourK_hi};
#endif

  NeonVec data0{Load(buf[0])};

#ifdef TJG_REFLECT_MODE
  data0 ^= NeonVec{Reflect(crc)};
#else
  data0 ^= NeonVec{U128{crc} << (128-32)};
#endif
#endif

#if 0
  // LSB-first fold constants for P*(x)=0xEDB88320
  constexpr std::uint32_t SingleK_lo = 0xF1CAE13E; // x^128 mod P*, lane0
  constexpr std::uint32_t SingleK_hi = 0x86D07BAD; // x^128 mod P*, lane1
  constexpr std::uint32_t FourK_lo   = 0x9F6E2336; // x^512 mod P*, lane0
  constexpr std::uint32_t FourK_hi   = 0x76E85057; // x^512 mod P*, lane1
  static const NeonPoly SingleK{SingleK_hi, SingleK_lo};
  static const NeonPoly FourK  {  FourK_hi,   FourK_lo};
  NeonVec data0{Load(buf[0])};
  data0 ^= NeonVec{tjg::IntMath::Reflect(crc)};
#endif

  if (num >= 8) {
    NeonVec data1{Load(buf[1])};
    NeonVec data2{Load(buf[2])};
    NeonVec data3{Load(buf[3])};

    for ( ; num >= 8; num -= 4) {
      buf += 4;
      data0 = TjgMulDiag(data0, FourK) ^ NeonVec{Load(buf[0])};
      data1 = TjgMulDiag(data1, FourK) ^ NeonVec{Load(buf[1])};
      data2 = TjgMulDiag(data2, FourK) ^ NeonVec{Load(buf[2])};
      data3 = TjgMulDiag(data3, FourK) ^ NeonVec{Load(buf[3])};
    }

    data0 = TjgMulDiag(data0, SingleK) ^ data1;
    data0 = TjgMulDiag(data0, SingleK) ^ data2;
    data0 = TjgMulDiag(data0, SingleK) ^ data3;
    num -= 3;
    buf += 3;
  }
  for ( ; num >= 2; --num)
    data0 = TjgMulDiag(data0, SingleK) ^ NeonVec{Load(*++buf)};
  return Load(U128{data0});
} // do_cksum_vmull

std::uint32_t cksum_vmull(std::uint32_t crc, const void* buf, size_t size)
  noexcept
{
  auto n = size / sizeof(U128);
  auto r = size % sizeof(U128);
  if (n < 2) {
    crc = std::byteswap(crc);
    crc = CrcUpdate(crc, buf, size);
    return std::byteswap(crc);
  }
  auto p = reinterpret_cast<const U128*>(buf);
  auto u = do_cksum_vmull(crc, p, n);
  crc = CrcUpdate(0, &u, sizeof(u));
  crc = CrcUpdate(crc, p+n, r);
  return std::byteswap(crc);
} // cksum_vmull

