#include "CrcUpdate.hpp"
#include "cksum.hpp"
#include "Int.hpp"
#include <gsl/gsl>
#include <bit>
#include <cstdint>

using Big32 = tjg::BigUint32;

template<int T>
inline Big32 Table(Big32 x) noexcept {
  auto y = CrcTab[T][std::to_integer<int>(std::byte(x))];
  return Big32::Raw(y);
}

using BufType = tjg::Int<std::uint64_t, std::endian::big>;

Big32 do_cksum_slice8(Big32 crc, const BufType* buf, std::size_t num)
  noexcept
{
  using std::uint8_t;
  while (num--) {
    auto data = *buf++;
    crc ^= Big32(data.rightBytes(4));
    auto low = Big32(data);
    crc = Table<7>(crc.rightBytes(3))
        ^ Table<6>(crc.rightBytes(2))
        ^ Table<5>(crc.rightBytes(1))
        ^ Table<4>(crc.rightBytes(0))
        ^ Table<3>(low.rightBytes(3))
        ^ Table<2>(low.rightBytes(2))
        ^ Table<1>(low.rightBytes(1))
        ^ Table<0>(low.rightBytes(0));
  }
  return crc;
} // do_cksum_slice8

CrcType cksum_slice8(CrcType crc, const void* buf, std::size_t size) noexcept {
  auto cp = reinterpret_cast<const std::byte*>(buf);
  { // Process unaligned head.
    auto r = reinterpret_cast<std::uintptr_t>(buf) % sizeof(std::uint64_t);
    if (r != 0) {
      r = sizeof(std::uint64_t) - r;
      crc = CrcUpdate(crc, cp, r);
      cp   += r;
      size -= r;
    }
  }
  auto n = size / sizeof(std::uint64_t);
  auto r = size % sizeof(std::uint64_t);
  auto ap = std::assume_aligned<sizeof(std::uint64_t)>(cp);
  auto p = reinterpret_cast<const BufType*>(ap);
  crc = do_cksum_slice8(Big32{crc}, p, n);
  crc = CrcUpdate(crc, p+n, r);
  return crc;
} // cksum_slice8

// Assumes callback provides int64_t-aligned data().
// Returns if size() is small or has a tail.
CrcType cksum_slice8(CrcType crc, GetBufferCb cb) noexcept {
  auto buf = cb();
  Expects(reinterpret_cast<std::uintptr_t>(buf.data()) % sizeof(std::uint64_t) == 0);
  if (buf.size() >= 2 * sizeof(std::uint64_t)) [[likely]] {
    auto state = Big32{crc};
    do {
      auto n = buf.size() / sizeof(std::uint64_t);
      auto r = buf.size() % sizeof(std::uint64_t);
      auto ap = std::assume_aligned<sizeof(std::uint64_t)>(buf.data());
      auto p = reinterpret_cast<const BufType*>(ap);
      state = do_cksum_slice8(state, p, n);
      if (r != 0) {
        buf = buf.last(r);
        break;
      }
      buf = cb();
      Expects(reinterpret_cast<std::uintptr_t>(buf.data()) % sizeof(std::uint64_t) == 0);
    } while (buf.size() >= sizeof(std::uint64_t));
    crc = state;
  }
  return CrcUpdate(crc, buf.data(), buf.size());
} // cksum_slice8 (callback)
