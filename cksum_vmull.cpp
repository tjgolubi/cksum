#include "CrcUpdate.hpp"
#include "cksum.hpp"
#include "Neon.hpp"

#include "Int.hpp"

#include <bit>

using U128 = tjg::Int<uint128_t, std::endian::big>;

U128 do_cksum_vmull(std::uint32_t crc, const U128* buf, std::size_t num)
  noexcept
{
  (void) tjg::VerifyInt<std::uint64_t>{};
  (void) tjg::VerifyInt<uint128_t>{};

  using NeonVec  = NeonV<uint64x2_t>;
  using NeonPoly = NeonV<poly64x2_t>;

  static const NeonPoly SingleK{0xe8a45605, 0xc5b9cd4c};
  static const NeonPoly FourK  {0xe6228b11, 0x8833794c};

  auto Load   = [&](U128 x) -> NeonVec { return NeonVec{x}; };
  auto Unload = [&](NeonVec x) -> U128 { return U128{x}; };

  NeonVec data0 = Load(buf[0]);

  data0 ^= NeonVec{uint128_t{crc} << (128-32)};

  if (num >= 8) {
    NeonVec data1 = Load(buf[1]);
    NeonVec data2 = Load(buf[2]);
    NeonVec data3 = Load(buf[3]);

    for ( ; num >= 8; num -= 4) {
      buf += 4;
      data0 = ClMult(data0, FourK) ^ Load(buf[0]);
      data1 = ClMult(data1, FourK) ^ Load(buf[1]);
      data2 = ClMult(data2, FourK) ^ Load(buf[2]);
      data3 = ClMult(data3, FourK) ^ Load(buf[3]);
    }

    data0 = ClMult(data0, SingleK) ^ data1;
    data0 = ClMult(data0, SingleK) ^ data2;
    data0 = ClMult(data0, SingleK) ^ data3;
    num -= 3;
    buf += 3;
  }
  for ( ; num >= 2; --num)
    data0 = ClMult(data0, SingleK) ^ NeonVec{*++buf};
  return Unload(data0);
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
