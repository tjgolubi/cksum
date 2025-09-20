#include "cksum.hpp"
#include "CrcConsts.hpp"

#include "CrcUpdate.hpp"
#include "Simd.hpp"

#include "Int.hpp"

#include <bit>

using simd::uint128_t;

using U128 = tjg::Int<uint128_t, std::endian::big>;
static_assert(alignof(U128) == alignof(uint128_t));

uint128_t do_cksum_simd(uint128_t init, const U128* buf, std::size_t num)
  noexcept
{
  using Vec = simd::Simd<simd::uint64x2_t>;
  using C = tjg::crc::Crc32Consts;

  static const auto SingleK = Vec{C::K128_lo, C::K128_hi};
  static const auto FourK   = Vec{C::K512_lo, C::K512_hi};

  auto Load   = [](U128 x) -> Vec  { return  Vec{x}; };

  auto data0 = Vec{init} ^ Load(buf[0]);

  if (num >= 8) {
    auto data1 = Load(buf[1]);
    auto data2 = Load(buf[2]);
    auto data3 = Load(buf[3]);

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
  return uint128_t{data0};
} // do_cksum_simd

CrcType cksum_simd(CrcType crc, const void* buf, std::size_t size) noexcept {
  if (size < 2 * sizeof(U128))
    return CrcUpdate(crc, buf, size);
  auto bp = reinterpret_cast<const std::byte*>(buf);
  {
    auto head =
        std::size_t{reinterpret_cast<std::uintptr_t>(bp) % alignof(uint128_t)};
    if (head != 0) [[unlikely]] {
      head  = alignof(uint128_t) - head;
      crc   = CrcUpdate(crc, bp, head);
      bp   += head;
      size -= head;
    }
  }
  auto ap = std::assume_aligned<alignof(uint128_t)>(bp);
  auto p  = reinterpret_cast<const U128*>(ap);
  auto n  = size / sizeof(U128);
  auto r  = size % sizeof(U128);
  auto u  = do_cksum_simd(uint128_t{crc} << (128-32), p, n);
  crc = CrcType{0};
  for (std::size_t i = 0; i != sizeof(u); ++i)
    crc = CrcUpdate(crc, std::byte(u >> 8*((sizeof(u)-1)-i)));
  return CrcUpdate(crc, p+n, r);
} // cksum_simd
