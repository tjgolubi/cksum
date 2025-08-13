/* cksum -- calculate and print POSIX checksums and sizes of files
   Copyright (C) 1992-2025 Free Software Foundation, Inc. */

#include "cksum.h"

#include <sys/types.h>
#include <endian.h>
#include <errno.h>

#include <sys/auxv.h>
#include <asm/hwcap.h>

/* Number of bytes to read at once.  */
# define BUFLEN (1 << 16)

typedef bool (*cksum_fp_t) (FILE *, uint_fast32_t *, uintmax_t *);

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
}

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
}

static
bool cksum_slice8 (FILE *fp, uint_fast32_t *crc_out, uintmax_t *length_out) {
  uint32_t buf[BUFLEN / sizeof (uint32_t)];
  uint_fast32_t crc = 0;
  uintmax_t length = 0;
  size_t bytes_read;

  if (!fp || !crc_out || !length_out)
    return false;

  while ((bytes_read = fread (buf, 1, BUFLEN, fp)) > 0) {
    uint32_t *datap;

    if (length + bytes_read < length) {
      errno = EOVERFLOW;
      return false;
    }
    length += bytes_read;

    /* Process multiples of 8 bytes */
    datap = (uint32_t *)buf;
    while (bytes_read >= 8) {
      uint32_t first = *datap++, second = *datap++;
      crc ^= htobe32 (first);
      second = htobe32 (second);
      crc = crctab[7][(crc >> 24)    & 0xff]
          ^ crctab[6][(crc >> 16)    & 0xff]
          ^ crctab[5][(crc >>  8)    & 0xff]
          ^ crctab[4][(crc      )    & 0xff]
          ^ crctab[3][(second >> 24) & 0xff]
          ^ crctab[2][(second >> 16) & 0xff]
          ^ crctab[1][(second >>  8) & 0xff]
          ^ crctab[0][(second      ) & 0xff];
      bytes_read -= 8;
    }

    /* And finish up last 0-7 bytes in a byte by byte fashion */
    unsigned char *cp = (unsigned char *)datap;
    while (bytes_read--)
      crc = (crc << 8) ^ crctab[0][((crc >> 24) ^ *cp++) & 0xff];
    if (feof (fp))
      break;
  }

  *crc_out = crc;
  *length_out = length;

  return !ferror (fp);
}

/* Calculate the checksum and length in bytes of stream STREAM.
   Return -1 on error, 0 on success.  */

int crc_sum_stream(FILE *stream, void *resstream, uintmax_t *length) {
  uintmax_t total_bytes = 0;
  uint_fast32_t crc = 0;

  static cksum_fp_t cksum_fp;
  if (!cksum_fp)
    cksum_fp = pclmul_supported();
  if (!cksum_fp)
    cksum_fp = vmull_supported();
  if (!cksum_fp)
    cksum_fp = cksum_slice8;

  if (!cksum_fp(stream, &crc, &total_bytes))
    return -1;

  *length = total_bytes;

  for (; total_bytes; total_bytes >>= 8)
    crc = (crc << 8) ^ crctab[0][((crc >> 24) ^ total_bytes) & 0xff];
  crc = ~crc & 0xffffffff;

  unsigned int crc_out = crc;
  memcpy(resstream, &crc_out, sizeof crc_out);

  return 0;
}

/* Print the checksum and size to stdout.
   If ARGS is true, also print the FILE name.  */

void output_crc(char const *file, int binary_file, void const *digest, bool raw,
                bool tagged, unsigned char delim, bool args, uintmax_t length)
{
  if (raw) {
    /* Output in network byte order (big endian).  */
    uint32_t out_int = htobe32(*(uint32_t *)digest);
    fwrite(&out_int, 1, 32/8, stdout);
    return;
  }

  printf("%u %ju", *(unsigned int *)digest, length);
  if (args)
    printf(" %s", file);
  putchar(delim);
}
