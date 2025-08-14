/* cksum -- calculate and print POSIX checksums and sizes of files
   Copyright (C) 1992-2025 Free Software Foundation, Inc. */

#include "cksum.h"

#include <sys/types.h>
#include <errno.h>

#if USE_VMULL_CRC32
#include <sys/auxv.h>
//#include <asm/hwcap.h>
//#include <arm_neon.h>
#endif

#include <stdio.h>

/* Number of bytes to read at once.  */
#define BUFLEN (1 << 16)

typedef unsigned __int128 uint128_t;

typedef uint_fast32_t (*cksum_fp_t) (uint_fast32_t, void* buf, size_t* bufsize);

extern uint_fast32_t const crctab[8][256];

extern
uint_fast32_t cksum_slice8(uint_fast32_t crc, void* buf, size_t* bufsize);

extern
uint_fast32_t cksum_vmull(uint_fast32_t crc, void* buf, size_t* bufsize);

extern
uint_fast32_t cksum_pclmul(uint_fast32_t crc, void* buf, size_t* bufsize);

static cksum_fp_t pclmul_supported(void) {
#if USE_PCLMUL_CRC32
  bool pclmul_enabled = (__builtin_cpu_supports("pclmul") > 0
                      && __builtin_cpu_supports("avx")    > 0);
  if (cksum_debug) {
    fprintf(stderr, "%s\n", (pclmul_enabled ? "using pclmul hardware support"
                                            : "pclmul support not detected"));
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
  if (cksum_debug) {
    fprintf(stderr, "%s\n", (vmull_enabled ? "using vmull hardware support"
                                           : "vmull support not detected"));
  }
  if (vmull_enabled)
    return cksum_vmull;
#endif
  return nullptr;
} // vmull_supported

/* Calculate the checksum and length in bytes of stream STREAM.
   Return false on error, true on success.  */

bool crc_sum_stream(FILE *stream, unsigned* result, uintmax_t *length) {
  uintmax_t total_bytes = 0;
  uint_fast32_t crc = 0;
  size_t bytes_read = 0;
  uint128_t buf[BUFLEN / sizeof(uint128_t)];

  static cksum_fp_t cksum_fp;
  if (!cksum_fp)
    cksum_fp = pclmul_supported();
  if (!cksum_fp)
    cksum_fp = vmull_supported();
  if (!cksum_fp)
    cksum_fp = cksum_slice8;

  while ((bytes_read = fread(buf, 1, BUFLEN, stream)) > 0) {
    if (total_bytes + bytes_read < total_bytes) {
      errno = EOVERFLOW;
      return false;
    }
    total_bytes += bytes_read;
    size_t len = bytes_read;
    crc = cksum_fp(crc, buf, &len);
    char* cp = (char*) buf + (bytes_read-len);
    while (len--)
      crc = (crc << 8) ^ crctab[0][((crc >> 24) ^ *cp++) & 0xff];
    if (feof(stream))
      break;
  }

  if (ferror(stream))
    return false;

  *length = total_bytes;

  for ( ; total_bytes; total_bytes >>= 8)
    crc = (crc << 8) ^ crctab[0][((crc >> 24) ^ total_bytes) & 0xff];
  crc = ~crc & 0xffffffff;

  *result = (unsigned) crc;

  return true;
} // crc_sum_stream
