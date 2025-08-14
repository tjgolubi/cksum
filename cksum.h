/* Calculate checksums and sizes of files
   Copyright 2025 Free Software Foundation, Inc. */

#ifndef __CKSUM_H__
#define __CKSUM_H__

#include <stdio.h>
#include <stdint.h>

extern bool cksum_debug;

extern bool crc_sum_stream(FILE *stream, unsigned *result, uintmax_t *length);

#endif
