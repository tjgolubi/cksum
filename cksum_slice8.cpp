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
  auto n = size / sizeof(std::uint64_t);
  auto r = size % sizeof(std::uint64_t);
  auto p = reinterpret_cast<const BufType*>(buf);
  crc = do_cksum_slice8(Big32{crc}, p, n);
  crc = CrcUpdate(crc, p+n, r);
  return crc;
} // cksum_slice8
