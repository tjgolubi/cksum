/* Calculate checksums and sizes of files
   Copyright 2025 Free Software Foundation, Inc. */

#ifndef __CKSUM_H__
#define __CKSUM_H__

#include <stdio.h>
#include <stdint.h>

extern bool cksum_debug;

extern int crc_sum_stream(FILE *stream, void *resstream, uintmax_t *length);

extern
bool cksum_vmull(FILE *fp, uint_fast32_t *crc_out, uintmax_t *length_out);

extern
bool cksum_pclmul(FILE *fp, uint_fast32_t *crc_out, uintmax_t *length_out);

extern uint_fast32_t const crctab[8][256];

#endif
