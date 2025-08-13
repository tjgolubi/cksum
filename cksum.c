/* cksum -- calculate and print POSIX checksums and sizes of files
   Copyright (C) 1992-2025 Free Software Foundation, Inc. */

#include <config.h>

# include "cksum.h"

# include <sys/types.h>
# include <endian.h>
# include "system.h"

# ifdef USE_VMULL_CRC32
#  include <sys/auxv.h>
#  include <asm/hwcap.h>
# endif

# include "crc.h"

#undef fopen
#undef fread
#undef fwrite
#undef feof
#undef ferror
#undef fprintf
#undef fflush

/* Number of bytes to read at once.  */
# define BUFLEN (1 << 16)

typedef bool (*cksum_fp_t) (FILE *, uint_fast32_t *, uintmax_t *);

static cksum_fp_t
pclmul_supported (void)
{
# if USE_PCLMUL_CRC32 || GL_CRC_X86_64_PCLMUL
  bool pclmul_enabled = (0 < __builtin_cpu_supports ("pclmul")
                         && 0 < __builtin_cpu_supports ("avx"));
  if (cksum_debug) {
    fprintf(stderr, "%s\n", (pclmul_enabled ? "using pclmul hardware support"
                                            : "pclmul support not detected"));
    fflush(stderr);
  }
  if (pclmul_enabled)
    return cksum_pclmul;
# endif

  return nullptr;
}

#if 0
static cksum_fp_t
avx2_supported (void)
{
  /* AVX512 processors will not set vpclmulqdq unless they support
     the avx512 version, but it implies that the avx2 version
     is supported  */
# if USE_AVX2_CRC32
  bool avx2_enabled = (0 < __builtin_cpu_supports ("vpclmulqdq")
                       && 0 < __builtin_cpu_supports ("avx2"));
#if 0
  if (cksum_debug) {
    fprintf(stderr, "%s\n", (avx2_enabled ? "using avx2 hardware support"
                                          : "avx2 support not detected"));
    fflush(stderr);
  }
#endif
  if (avx2_enabled)
    return cksum_avx2;
# endif

  return nullptr;
}

static cksum_fp_t
avx512_supported (void)
{
  /* vpclmulqdq for multiplication
     mavx512f for most of the avx512 functions we're using
     mavx512bw for byte swapping  */
# if USE_AVX512_CRC32
  bool avx512_enabled = (0 < __builtin_cpu_supports ("vpclmulqdq")
                         && 0 < __builtin_cpu_supports ("avx512bw")
                         && 0 < __builtin_cpu_supports ("avx512f"));
#if 0
  if (cksum_debug) {
    fprintf(stderr, "%s\n", (avx512_enabled ? "using avx512 hardware support"
                                            : "avx512 support not detected"));
    fflush(stderr);
  }
#endif
  if (avx512_enabled)
    return cksum_avx512;
# endif

  return nullptr;
}
#endif

static cksum_fp_t
vmull_supported (void)
{
  /* vmull for multiplication  */
# if USE_VMULL_CRC32
  bool vmull_enabled = (getauxval (AT_HWCAP) & HWCAP_PMULL) > 0;
  if (cksum_debug) {
    fprintf(stderr, "%s\n", (vmull_enabled ? "using vmull hardware support"
                                           : "vmull support not detected"));
    fflush(stderr);
  }
  if (vmull_enabled)
    return cksum_vmull;
# endif

  return nullptr;
}

static bool
cksum_slice8 (FILE *fp, uint_fast32_t *crc_out, uintmax_t *length_out)
{
  uint32_t buf[BUFLEN / sizeof (uint32_t)];
  uint_fast32_t crc = 0;
  uintmax_t length = 0;
  size_t bytes_read;

  if (!fp || !crc_out || !length_out)
    return false;

  while ((bytes_read = fread (buf, 1, BUFLEN, fp)) > 0)
    {
      uint32_t *datap;

      if (length + bytes_read < length)
        {
          errno = EOVERFLOW;
          return false;
        }
      length += bytes_read;

      /* Process multiples of 8 bytes */
      datap = (uint32_t *)buf;
      while (bytes_read >= 8)
        {
          uint32_t first = *datap++, second = *datap++;
          crc ^= htobe32 (first);
          second = htobe32 (second);
          crc = (crctab[7][(crc >> 24) & 0xFF]
                 ^ crctab[6][(crc >> 16) & 0xFF]
                 ^ crctab[5][(crc >> 8) & 0xFF]
                 ^ crctab[4][(crc) & 0xFF]
                 ^ crctab[3][(second >> 24) & 0xFF]
                 ^ crctab[2][(second >> 16) & 0xFF]
                 ^ crctab[1][(second >> 8) & 0xFF]
                 ^ crctab[0][(second) & 0xFF]);
          bytes_read -= 8;
        }

      /* And finish up last 0-7 bytes in a byte by byte fashion */
      unsigned char *cp = (unsigned char *)datap;
      while (bytes_read--)
        crc = (crc << 8) ^ crctab[0][((crc >> 24) ^ *cp++) & 0xFF];
      if (feof (fp))
        break;
    }

  *crc_out = crc;
  *length_out = length;

  return !ferror (fp);
}

/* Calculate the checksum and length in bytes of stream STREAM.
   Return -1 on error, 0 on success.  */

int
crc_sum_stream (FILE *stream, void *resstream, uintmax_t *length)
{
  uintmax_t total_bytes = 0;
  uint_fast32_t crc = 0;

  static cksum_fp_t cksum_fp;
#if 0
  if (! cksum_fp)
    cksum_fp = avx512_supported ();
  if (! cksum_fp)
    cksum_fp = avx2_supported ();
#endif
  if (! cksum_fp)
    cksum_fp = pclmul_supported ();
  if (! cksum_fp)
    cksum_fp = vmull_supported ();
  if (! cksum_fp)
    cksum_fp = cksum_slice8;

  if (! cksum_fp(stream, &crc, &total_bytes))
    return -1;

  *length = total_bytes;

  for (; total_bytes; total_bytes >>= 8)
    crc = (crc << 8) ^ crctab[0][((crc >> 24) ^ total_bytes) & 0xFF];
  crc = ~crc & 0xFFFFFFFF;

  unsigned int crc_out = crc;
  memcpy (resstream, &crc_out, sizeof crc_out);

  return 0;
}

#if 0
/* Calculate the crc32b checksum and length in bytes of stream STREAM.
   Return -1 on error, 0 on success.  */

int
crc32b_sum_stream (FILE *stream, void *resstream, uintmax_t *reslen)
{
  uint32_t buf[BUFLEN / sizeof (uint32_t)];
  uint32_t crc = 0;
  uintmax_t len = 0;
  size_t bytes_read;

  if (!stream || !resstream || !reslen)
    return -1;

# if GL_CRC_X86_64_PCLMUL
  if (cksum_debug)
    (void) pclmul_supported ();
# endif

  while ((bytes_read = fread (buf, 1, BUFLEN, stream)) > 0)
    {
      if (len + bytes_read < len)
        {
          errno = EOVERFLOW;
          return -1;
        }
      len += bytes_read;

      crc = crc32_update (crc, (char const *)buf, bytes_read);

      if (feof (stream))
        break;
    }

  unsigned int crc_out = crc;
  memcpy (resstream, &crc_out, sizeof crc_out);

  *reslen = len;

  return ferror (stream) ? -1 : 0;
}
#endif

/* Print the checksum and size to stdout.
   If ARGS is true, also print the FILE name.  */

void
output_crc (char const *file, int binary_file, void const *digest, bool raw,
            bool tagged, unsigned char delim, bool args, uintmax_t length)
{
  if (raw)
    {
      /* Output in network byte order (big endian).  */
      uint32_t out_int = htobe32 (*(uint32_t *)digest);
      fwrite (&out_int, 1, 32/8, stdout);
      return;
    }

  printf ("%u %ju", *(unsigned int *)digest, length);
  if (args)
    printf (" %s", file);
  putchar (delim);
}
