/* cksum -- calculate and print POSIX checksums and sizes of files
   Copyright (C) 1992-2025 Free Software Foundation, Inc. */

#include "cksum.h"

#include <sys/types.h>
#include <errno.h>

#include <stdio.h>

extern uint_fast32_t const crctab[8][256];

#if defined(__GNUC__) || defined(__clang__)
static inline uint32_t ToBigEndian(uint32_t x) {
  return __builtin_bswap32(x);
} // ToBigEndian
#endif

uint_fast32_t cksum_slice8(uint_fast32_t crc, void* buf, size_t* buflen) {
  /* Process multiples of 8 bytes */
  const uint32_t* datap = (const uint32_t*) buf;
  size_t num  = *buflen / 8;
  while (num >= 1) {
    uint32_t first  = *datap++;
    uint32_t second = *datap++;
    crc   ^= ToBigEndian(first);
    second = ToBigEndian(second);
    crc = crctab[7][(   crc >> 24) & 0xff]
        ^ crctab[6][(   crc >> 16) & 0xff]
        ^ crctab[5][(   crc >>  8) & 0xff]
        ^ crctab[4][(   crc >>  0) & 0xff]
        ^ crctab[3][(second >> 24) & 0xff]
        ^ crctab[2][(second >> 16) & 0xff]
        ^ crctab[1][(second >>  8) & 0xff]
        ^ crctab[0][(second >>  0) & 0xff];
    --num;
  }

  *buflen = *buflen % 8 + 8 * num;
  return crc;
} // cksum_slice8
