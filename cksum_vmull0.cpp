#include "cksum.hpp"

#include "CrcUpdate.hpp"

#include <bit>

#include <arm_neon.h>

using uint128_t = unsigned __int128;

constexpr uint64x2_t bswap_neon(uint64x2_t in) noexcept {
  uint64x2_t a = vreinterpretq_u64_u8(vrev64q_u8(vreinterpretq_u8_u64(in)));
  return vcombine_u64(vget_high_u64(a), vget_low_u64(a));
}

uint128_t do_cksum_vmull0(std::uint32_t crc, const uint128_t* buf, std::size_t num)
  noexcept
{
  auto OneK  = vcombine_p64(vcreate_p64(0xE8A45605), vcreate_p64(0xC5B9CD4C));
  auto FourK = vcombine_p64(vcreate_p64(0xE6228B11), vcreate_p64(0x8833794C));

  auto Load = [](const uint128_t* p) -> uint64x2_t {
    auto v = vld1q_u64(reinterpret_cast<const std::uint64_t*>(p));
    return bswap_neon(v);
  };

  auto Unload = [](uint64x2_t v) -> uint128_t {
    v = bswap_neon(v);
    return std::bit_cast<uint128_t>(v);
  };

  auto ClMulDiag = [](uint64x2_t x, poly64x2_t k) {
    auto t = vreinterpretq_u64_p128(
                    vmull_p64(vgetq_lane_p64(vreinterpretq_p64_u64(x), 0),
                              vgetq_lane_p64(k, 0)));
    x = vreinterpretq_u64_p128(vmull_high_p64(vreinterpretq_p64_u64(x), k));
    return veorq_u64(x, t);
  };

  auto d0 = Load(buf+0);
  auto xor_crc =
            vcombine_u64(vcreate_u64(0), vcreate_u64(std::uint64_t{crc} << 32));
  d0 = veorq_u64(d0, xor_crc);
  if (num >= 8) {
    auto d1 = Load(buf+1);
    auto d2 = Load(buf+2);
    auto d3 = Load(buf+3);
    for ( ; num >= 8; num -= 4) {
      buf += 4;

      d0 = ClMulDiag(d0, FourK);
      d1 = ClMulDiag(d1, FourK);
      d2 = ClMulDiag(d2, FourK);
      d3 = ClMulDiag(d3, FourK);

      auto d0a = Load(buf+0);
      d0 = veorq_u64(d0, d0a);

      auto d1a = Load(buf+1);
      d1 = veorq_u64(d1, d1a);

      auto d2a = Load(buf+2);
      d2 = veorq_u64(d2, d2a);

      auto d3a = Load(buf+3);
      d3 = veorq_u64(d3, d3a);
    }
    d0 = ClMulDiag(d0, OneK);
    d0 = veorq_u64(d0, d1);
    d0 = ClMulDiag(d0, OneK);
    d0 = veorq_u64(d0, d2);
    d0 = ClMulDiag(d0, OneK);
    d0 = veorq_u64(d0, d3);
    num -= 3;
    buf += 3;
  }
  for ( ; num >= 2; --num) {
    d0 = ClMulDiag(d0, OneK);
    auto t = Load(++buf);
    d0 = veorq_u64(d0, t);
  }
  return Unload(d0);
} // do_cksum_vmull0

std::uint32_t cksum_vmull0(std::uint32_t crc, const void* buf, std::size_t size)
  noexcept
{
  auto n = size / sizeof(uint128_t);
  auto r = size % sizeof(uint128_t);
  if (n < 2) {
    crc = std::byteswap(crc);
    crc = CrcUpdate(crc, buf, size);
    return std::byteswap(crc);
  }
  auto p = reinterpret_cast<const uint128_t*>(buf);
  auto u = do_cksum_vmull0(crc, p, n);
  crc = CrcUpdate(0, &u, sizeof(u));
  crc = CrcUpdate(crc, p+n, r);
  return std::byteswap(crc);
} // cksum_vmull0
