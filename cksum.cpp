/* cksum -- calculate and print POSIX checksums and sizes of files
   Copyright (C) 1992-2025 Free Software Foundation, Inc. */

#include "cksum.hpp"

#include <iostream>

//#include <sys/types.h>

#if USE_VMULL_CRC32
#include <sys/auxv.h>
#endif

/* Number of bytes to read at once.  */
constexpr std::size_t BufLen = 1 << 16;

typedef uint_fast32_t (*cksum_fp_t) (uint_fast32_t, void* buf, size_t* bufsize);

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
    return cksum_pclmul;
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
    return cksum_vmull;
#endif
  return nullptr;
} // vmull_supported

/* Calculate the checksum and length in bytes of stream STREAM.
   Return false on error, true on success.  */

std::uint32_t CrcSumStream(std::ifstream& stream, std::streamsize* length) {
  static cksum_fp_t cksum_fp;
  if (!cksum_fp)
    cksum_fp = pclmul_supported();
  if (!cksum_fp)
    cksum_fp = vmull_supported();
  if (!cksum_fp)
    cksum_fp = cksum_slice8;

  auto crc = uint_fast32_t{0};
  auto total_bytes = std::streamsize{0};
  auto buf = std::array<uint128_t, BufLen/sizeof(uint128_t)>{};
  auto cbuf = reinterpret_cast<char*>(buf.data());

  stream.exceptions(std::ios::badbit);
  do {
    stream.read(cbuf, BufLen);
    auto bytes_read = stream.gcount();
    if (bytes_read == 0)
      break;
    if (total_bytes + bytes_read < total_bytes)
      throw std::overflow_error{"Failure reading input stream"};
    total_bytes += bytes_read;
    auto len = static_cast<std::size_t>(bytes_read);
    crc = cksum_fp(crc, buf.data(), &len);
    auto cp = cbuf + (bytes_read-len);
    while (len--)
      crc = (crc << 8) ^ CrcTab[0][((crc >> 24) ^ *cp++) & 0xff];
  } while (!stream.eof());

  if (length)
    *length = total_bytes;

  for ( ; total_bytes; total_bytes >>= 8)
    crc = (crc << 8) ^ CrcTab[0][((crc >> 24) ^ total_bytes) & 0xff];
  crc = ~crc & 0xffffffff;

  return crc;
} // CrcSumStream
