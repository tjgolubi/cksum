#include "CrcUpdate.hpp"
#include "cksum.hpp"

#include <iostream>

#if USE_VMULL_CRC32
#include <sys/auxv.h>
#endif

/* Number of bytes to read at once.  */
constexpr std::size_t BufLen = 1 << 16;

using cksum_fp_t = CrcType (*)(CrcType crc, GetBufferCb cb);

static cksum_fp_t pclmul_supported(void) {
#if USE_PCLMUL_CRC32
  bool pclmul_enabled = (__builtin_cpu_supports("pclmul") > 0
                      && __builtin_cpu_supports("avx")    > 0);
  if (CksumDebug) {
    if (pclmul_enabled)
      std::cerr << "using pclmul hardware support\n";
    else
      std::cerr << "pclmul support not detected\n";
  }
  if (pclmul_enabled)
    return cksum_simd;
#endif
  return nullptr;
} // pclmul_supported

static cksum_fp_t vmull_supported(void) {
  /* vmull for multiplication  */
#if USE_VMULL_CRC32
  bool vmull_enabled = (getauxval(AT_HWCAP) & HWCAP_PMULL) > 0;
  if (CksumDebug) {
    if (vmull_enabled)
      std::cerr << "using vmull hardware support\n";
    else
      std::cerr << "vmull support not detected\n";
  }
  if (vmull_enabled)
    return cksum_simd;
#endif
  return nullptr;
} // vmull_supported

CrcType cksum_update(CrcType crc, const void* buf, std::size_t size) noexcept {
  return CrcUpdate(crc, buf, size);
}

CrcType cksum_update(CrcType crc, GetBufferCb cb) noexcept {
  auto buf = cb();
  while (!buf.empty()) {
    crc = CrcUpdate(crc, buf.data(), buf.size());
    buf = cb();
  }
  return crc;
}

/* Calculate the checksum and length in bytes of stream STREAM.
   Return false on error, true on success.  */

CrcType CrcSumStream(std::ifstream& stream, std::streamsize* length) {
  static cksum_fp_t cksum_fp;
  if (!cksum_fp)
    cksum_fp = pclmul_supported();
  if (!cksum_fp)
    cksum_fp = vmull_supported();
  if (!cksum_fp)
    cksum_fp = cksum_slice8;

  auto total_bytes = std::streamsize{0};
  using uint128_t = unsigned __int128;
  alignas(uint128_t) auto buf = std::array<std::byte, BufLen>{};
  auto cbuf = reinterpret_cast<char*>(buf.data());

  auto get_buffer = [&]() noexcept -> std::span<const std::byte> {
    using RType = std::span<const std::byte>;
    if (!stream)
      return RType{};
    stream.read(cbuf, BufLen);
    auto bytes_read = stream.gcount();
    if (bytes_read == 0)
      return RType{};
    // if (total_bytes + bytes_read < total_bytes)
    //   throw std::overflow_error{"Failure reading input stream"};
    total_bytes += bytes_read;
    auto len = static_cast<std::size_t>(bytes_read);
    return RType{buf.data(), len};
  };

  auto crc = cksum_fp(CrcType{0}, get_buffer);

  if (length)
    *length = total_bytes;

  for ( ; total_bytes; total_bytes >>= 8)
    crc = CrcUpdate(crc, std::byte(total_bytes));
  return ~crc;
} // CrcSumStream
