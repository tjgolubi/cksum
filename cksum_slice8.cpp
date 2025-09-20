#include "CrcUpdate.hpp"
#include "cksum.hpp"
#include "Int.hpp"
#include <bit>
#include <cstdint>

using Big32 = tjg::BigUint32;

template<int T>
inline Big32 Table(Big32 x) noexcept {
  auto y = CrcTab[T][std::to_integer<int>(std::byte(x))];
  return Big32::Raw(y);
}

using BufType = tjg::Int<std::uint64_t, std::endian::big>;

CrcType do_cksum_slice8(Big32 crc, const BufType* buf, std::size_t num)
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
  using std::uint64_t;
  if (size < sizeof(uint64_t))
    return CrcUpdate(crc, buf, size);
  auto bp = reinterpret_cast<const std::byte*>(buf);
  {
    auto head =
          std::size_t{reinterpret_cast<std::uintptr_t>(bp) % alignof(uint64_t)};
    if (head != 0) {
      head  = alignof(uint64_t) - head;
      crc   = CrcUpdate(crc, bp, head);
      bp   += head;
      size -= head;
    }
  }
  auto ap = std::assume_aligned<alignof(uint64_t)>(bp);
  auto p  = reinterpret_cast<const BufType*>(ap);
  auto n  = size / sizeof(uint64_t);
  auto r  = size % sizeof(uint64_t);
  crc = do_cksum_slice8(Big32{crc}, p, n);
  return CrcUpdate(crc, p+n, r);
} // cksum_slice8
