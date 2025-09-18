#include "cksum.hpp"
#include "CrcConsts.hpp"

#include "CrcUpdate.hpp"
#include "Simd.hpp"

#include "Int.hpp"

#include <bit>

using simd::uint128_t;

using U128 = tjg::Int<uint128_t, std::endian::big>;

using Vec = simd::Simd<simd::uint64x2_t>;
using C = tjg::crc::Crc32Consts;

static const auto SingleK = Vec{C::K128_lo, C::K128_hi};
static const auto FourK   = Vec{C::K512_lo, C::K512_hi};

uint128_t do_cksum_simd(uint128_t init, const U128* buf, std::size_t num)
  noexcept
{
  auto Load = [](U128 x) -> Vec  { return  Vec{x}; };

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
  auto n = size / sizeof(U128);
  auto r = size % sizeof(U128);
  if (n < 2)
    return CrcUpdate(crc, buf, size);
  auto p = reinterpret_cast<const U128*>(buf);
  auto u = do_cksum_simd(uint128_t{crc} << (128-32), p, n);
  crc = CrcType{0};
  for (std::size_t i = 0; i != sizeof(u); ++i)
    crc = CrcUpdate(crc, std::byte(u >> 8*((sizeof(u)-1)-i)));
  crc = CrcUpdate(crc, p+n, r);
  return crc;
} // cksum_simd

CrcType cksum_simd(CrcType crc, GetBufferCb cb) noexcept {
  auto buf = cb();
  if (buf.empty())
    return crc;
  if (buf.size() < 2 * sizeof(uint128_t))
    return CrcUpdate(crc, buf.data(), buf.size());
  auto state = uint128_t{crc} << (128-32);
  for (;;) {
    auto n = buf.size() / sizeof(uint128_t);
    auto r = buf.size() % sizeof(uint128_t);
    auto p = reinterpret_cast<const U128*>(buf.data());
    state = do_cksum_simd(state, p, n);
    if (r != 0) {
      buf = buf.last(r);
      break;
    }
    buf = cb();
    if (buf.size() < 2 * sizeof(uint128_t))
      break;
    state = ClMulDiag(Vec{state}, SingleK);
  }
  crc = CrcType{0};
  for (std::size_t i = 0; i != sizeof(state); ++i)
    crc = CrcUpdate(crc, std::byte(state >> 8*((sizeof(uint128_t)-1)-i)));
  return CrcUpdate(crc, buf.data(), buf.size());
} // cksum_simd
