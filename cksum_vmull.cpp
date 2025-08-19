/* cksum -- calculate and print POSIX checksums and sizes of files
   Copyright (C) 2024-2025 Free Software Foundation, Inc. */

#include "cksum.hpp"
#include "Neon.hpp"
#include "Int.hpp"

#include <array>

#include <sys/types.h>
#include <arm_neon.h>

#include <iostream>
#include <iomanip>

/* Calculate CRC32 using VMULL CPU instruction found in ARMv8 CPUs */

uint_fast32_t cksum_vmull(uint_fast32_t crc, void* buf, size_t* bufsize)
  noexcept
{
  using U128 = tjg::BigUint<uint128_t>;
  static_assert(sizeof(U128)  == sizeof(uint128_t));
  static_assert(alignof(U128) == alignof(uint128_t));

  auto num = *bufsize / sizeof(uint128_t);

  using NeonVec  = NeonV<uint64x2_t>;
  using NeonPoly = NeonV<poly64x2_t>;

  /* These constants and general algorithms are taken from the Intel whitepaper
     "Fast CRC Computation for Generic Polynomials Using PCLMULQDQ Instruction"
     */
  static const auto SingleK_Init = std::array<poly64_t, 2>{
                                  {poly64_t{0xe8a45605}, poly64_t{0xc5b9cd4c}}};
  static const auto FourK_Init   = std::array<poly64_t, 2>{
                                  {poly64_t{0xe6228b11}, poly64_t{0x8833794c}}};
  static const NeonPoly SingleK{SingleK_Init};
  static const NeonPoly FourK  {FourK_Init};

  auto datap = static_cast<U128*>(buf);

  NeonVec data0;
  NeonVec data1;
  NeonVec data2;
  NeonVec data3;

  /* Fold in parallel eight 16-byte blocks into four 16-byte blocks */
  if (num >= 8) {
    data0 = datap[0];
    data1 = datap[1];
    data2 = datap[2];
    data3 = datap[3];

    if (crc != 0) {
      data0 ^= NeonVec{std::uint64_t{0}, std::uint64_t{crc} << 32};
      crc = 0;
    }

    for ( ; num >= 8; num -= 4) {
      datap += 4;
      data0 = ClMult(data0, FourK) ^ NeonVec{datap[0]};
      data1 = ClMult(data1, FourK) ^ NeonVec{datap[1]};
      data2 = ClMult(data2, FourK) ^ NeonVec{datap[2]};
      data3 = ClMult(data3, FourK) ^ NeonVec{datap[3]};
    }
    /* At end of loop we write out results from variables back into
       the buffer, for use in single fold loop */
    // data0.get(&datap[0]);
    // data1.get(&datap[1]);
    // data2.get(&datap[2]);
    // data3.get(&datap[3]);

    data0 = ClMult(data0, SingleK) ^ data1;
    data0 = ClMult(data0, SingleK) ^ data2;
    data0 = ClMult(data0, SingleK) ^ data3;
    num   -= 3;
    datap += 3;
    for ( ; num >= 2; --num)
      data0 = ClMult(data0, SingleK) ^ NeonVec{*++datap};
    data0.get(&datap[0]);
    *bufsize = *bufsize % sizeof(uint128_t) + sizeof(uint128_t);
    return 0;
  }
  else if (num >= 2) {
    /* Fold two 16-byte blocks into one 16-byte block */
    data0 = datap[0];
    if (crc != 0) {
      data0 ^= NeonVec{std::uint64_t{0}, std::uint64_t{crc} << 32};
      crc = 0;
    }
    for ( ; num >= 2; --num)
      data0 = ClMult(data0, SingleK) ^ NeonVec{*++datap};
    data0.get(&datap[0]);
    *bufsize = *bufsize % sizeof(uint128_t) + sizeof(uint128_t);
    return 0;
  }
  else if (num == 1) {
    datap[0] ^= uint128_t{crc} << (32+64);
    *bufsize = *bufsize % sizeof(uint128_t) + sizeof(uint128_t);
    return 0;
  }
  else {
    *bufsize = *bufsize % sizeof(uint128_t);
    return crc;
  }
} // cksum_vmull
