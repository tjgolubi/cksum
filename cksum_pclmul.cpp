#include "cksum.hpp"

#include "CrcUpdate.hpp"
#include "CrcConsts.hpp"
#include "Int.hpp"
#include "Simd.hpp"

#include <concepts>
#include <type_traits>
#include <bit>

using uint128_t = unsigned __int128;

using U128 = tjg::Int<uint128_t, std::endian::big>;

U128 do_cksum_pclmul(std::uint32_t crc, const U128* buf, size_t num)
  noexcept
{
  using Vec = simd::Simd<simd::uint64x2_t>;
  using C = tjg::crc::CrcConsts<32, 0x04c11db7>;
  using simd::ClMulDiag;

  static const auto SingleK = Vec{C::K128_lo, C::K128_hi};
  static const auto FourK   = Vec{C::K512_lo, C::K512_hi};

  auto Load   = [](U128 x) -> Vec  { return  Vec{x}; };
  auto Unload = [](Vec  x) -> U128 { return U128{x}; };

  auto data0 = Load(buf[0]);

  data0 ^= Vec{uint128_t{crc} << (128-32)};

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
