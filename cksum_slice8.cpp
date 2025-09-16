#include "CrcUpdate.hpp"
#include "cksum.hpp"
#include <cstdint>

template<int T>
inline CrcType Table(CrcType x) noexcept {
  auto y = CrcTab[T][std::to_integer<int>(std::byte(x))];
  return CrcType::Raw(y);
}

CrcType do_cksum_slice8(CrcType crc, const std::uint64_t* buf, std::size_t num)
  noexcept
{
  using std::uint8_t;
  auto p = reinterpret_cast<const CrcType*>(buf);
  while (num--) {
    crc ^= *p++;
    auto low = *p++;
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
  auto p = reinterpret_cast<const std::uint64_t*>(buf);
  crc = do_cksum_slice8(crc, p, n);
  crc = CrcUpdate(crc, p+n, r);
  return crc;
} // cksum_slice8
