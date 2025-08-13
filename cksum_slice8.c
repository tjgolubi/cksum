/* cksum -- calculate and print POSIX checksums and sizes of files
   Copyright (C) 1992-2025 Free Software Foundation, Inc. */

#include "cksum.h"

#include <stdio.h>
#include <errno.h>
#include <endian.h>

/* Number of bytes to read at once.  */
#define BUFLEN (1 << 16)

extern uint_fast32_t const crctab[8][256];

bool cksum_slice8(FILE *fp, uint_fast32_t *crc_out, uintmax_t *length_out) {
  uint32_t buf[BUFLEN / sizeof (uint32_t)];
  uint_fast32_t crc = 0;
  uintmax_t length = 0;
  size_t bytes_read;

  if (!fp || !crc_out || !length_out)
    return false;

  while ((bytes_read = fread(buf, 1, BUFLEN, fp)) > 0) {
    uint32_t *datap;

    if (length + bytes_read < length) {
      errno = EOVERFLOW;
      return false;
    }
    length += bytes_read;

    /* Process multiples of 8 bytes */
    datap = (uint32_t *)buf;
    while (bytes_read >= 8) {
      uint32_t first  = *datap++;
      uint32_t second = *datap++;
      crc ^= htobe32(first);
      second = htobe32(second);
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
    if (feof(fp))
      break;
  }

  *crc_out = crc;
  *length_out = length;

  return !ferror(fp);
} // cksum_slice8
