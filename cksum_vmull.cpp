#include "CrcUpdate.hpp"
#include "Reflect.hpp"
#include "cksum.hpp"
#include "Neon.hpp"

#include "Int.hpp"

#include <bit>

constexpr uint128_t Uint128(std::uint64_t hi, std::uint64_t lo) noexcept
  { return (uint128_t{hi} << 64) | uint128_t{lo}; }

namespace tjg::IntMath {

constexpr uint128_t Reflect(uint128_t x) noexcept {
  auto lo = static_cast<std::uint64_t>(x);
  auto hi = static_cast<std::uint64_t>(x >> 64);
  return Uint128(Reflect(lo), Reflect(hi));
}

} // tjg::IntMath

using U128 = uint128_t;

U128 do_cksum_vmull(std::uint32_t crc, const U128* buf, std::size_t num)
  noexcept
{
  using tjg::IntMath::Reflect;
  using std::byteswap;

  using NeonVec  = NeonV<uint64x2_t>;
  using NeonPoly = NeonV<poly64x2_t>;

  constexpr std::uint32_t SingleK_lo = 0xe8a45605;
  constexpr std::uint32_t SingleK_hi = 0xc5b9cd4c;
  constexpr std::uint32_t FourK_lo   = 0xe6228b11;
  constexpr std::uint32_t FourK_hi   = 0x8833794c;

  static const NeonPoly SingleK{SingleK_lo, SingleK_hi};
  static const NeonPoly FourK  {  FourK_lo,   FourK_hi};

  NeonVec data0{byteswap(buf[0])};

  data0 ^= NeonVec{uint128_t{crc} << (128-32)};

  if (num >= 8) {
    NeonVec data1{byteswap(buf[1])};
    NeonVec data2{byteswap(buf[2])};
    NeonVec data3{byteswap(buf[3])};

    for ( ; num >= 8; num -= 4) {
      buf += 4;
      data0 = ClMulDiag(data0, FourK) ^ NeonVec{byteswap(buf[0])};
      data1 = ClMulDiag(data1, FourK) ^ NeonVec{byteswap(buf[1])};
      data2 = ClMulDiag(data2, FourK) ^ NeonVec{byteswap(buf[2])};
      data3 = ClMulDiag(data3, FourK) ^ NeonVec{byteswap(buf[3])};
    }

    data0 = ClMulDiag(data0, SingleK) ^ data1;
    data0 = ClMulDiag(data0, SingleK) ^ data2;
    data0 = ClMulDiag(data0, SingleK) ^ data3;
    num -= 3;
    buf += 3;
  }
  for ( ; num >= 2; --num)
    data0 = ClMulDiag(data0, SingleK) ^ NeonVec{byteswap(*++buf)};
  return byteswap(U128{data0});
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
