// gen_crc32_lsb_constants_clmul+bit_fixed.cpp
// Correct derivation of LSB-first fold constants (SingleK/FourK) for CRC-32.
// Uses your 64x64->128 ClMul for residues; solves ONE 1024x32 system per lane with bit::gauss.
//
// Build:
//   g++ -std=c++20 -O2 gen_crc32_lsb_constants_clmul+bit_fixed.cpp -I . -o gen && ./gen

#include "../ClMul/ClMulNeon.hpp"

#include <bit/matrix.h>
#include <bit/gauss.h>
#include <bit/vector.h>

#include <cstdint>
#include <array>
#include <bit>
#include <cstdio>

using u32 = std::uint32_t;
using u64 = std::uint64_t;
using u128 = unsigned __int128;

// P*(x) = x^32 + 0xEDB88320 (LSB-first reciprocal)
static constexpr u64 PSTAR = (1ull << 32) | 0xEDB88320ull;

// Reduce 128-bit polynomial modulo P* (degree 32), LSB-first
static constexpr u32 ReduceModPstar(u128 p) {
  while ((p >> 32) != 0) {
    int k;
    if (u64 hi = static_cast<u64>(p >> 64); hi != 0) k = 64 + (std::bit_width(hi) - 1);
    else { u64 lo = static_cast<u64>(p); k = std::bit_width(lo) - 1; }
    p ^= (static_cast<u128>(PSTAR) << (k - 32));
  }
  return static_cast<u32>(p);
}

// Multiply/reduce in GF(2)[x]
static constexpr u32 MulModPstar(u64 a, u64 b) { return ReduceModPstar(neon::ClMul(a, b)); }

// x^n mod P* via fast exponentiation
static constexpr u32 XpowMod(unsigned n) {
  u64 x = 2ull; // x^1
  u64 r = 1ull; // x^0
  while (n) {
    if (n & 1) r = MulModPstar(r, x);
    x = MulModPstar(x, x);
    n >>= 1;
  }
  return static_cast<u32>(r);
}

// Target residue for fold-by-N on basis 128-bit input (lo,hi)
static inline u32 TargetResidue(unsigned N, u64 lo, u64 hi) {
  std::array<u32,128> xp{};
  for (unsigned k = 0; k < 128; ++k) xp[k] = XpowMod(N + k);
  u32 acc = 0;
  auto add_bits = [&](u64 w, unsigned base) {
    while (w) {
      unsigned j = std::countr_zero(w);
      acc ^= xp[base + j];
      w &= (w - 1);
    }
  };
  add_bits(lo, 0);
  add_bits(hi, 64);
  return acc;
}

// Solve for 32-bit lane constant K (lane_index 0/1) via ONE 1024x32 system:
// Stack all 32 equations M_i * K = v_i into M*K = b and solve.
static u32 SolveLaneK(unsigned N, int lane_index) {
  // Build big matrix M (1024 rows, 32 cols) and RHS b (1024 bits)
  bit::matrix<> M(32 * 32, 32);
  bit::vector<> b(32 * 32);

  for (u32 i = 0; i < 32; ++i) {
    // v_i for basis input in this lane under fold-by-N
    u64 lo = 0, hi = 0;
    if (lane_index == 0) lo = (1ull << i); else hi = (1ull << i);
    u32 v_i = TargetResidue(N, lo, hi);

    // Build M_i rows: row (i*32 + t) has columns j set where bit t of XpowMod(i+j) is 1
    for (u32 t = 0; t < 32; ++t) {
      const u32 row = i * 32 + t;
      for (u32 j = 0; j < 32; ++j) {
        if ( (XpowMod(i + j) >> t) & 1u )
          M.set(row, j);
      }
      if ( (v_i >> t) & 1u ) b.set(row);
    }
  }

  // Solve M * K = b
  bit::gauss solver(M, b);
  auto Kvec = solver();  // 32-bit solution vector

  // Pack to 32-bit word: bit j of Kvec is bit j of constant K
  u32 K = 0;
  for (u32 j = 0; j < 32; ++j)
    if (Kvec[j]) K |= (1u << j);
  return K;
}

int main() {
  // Sanity: x^n mod P* for n=0..40 (n<32 must equal 1<<n)
  std::puts("Sanity: x^n mod P* for n=0..40");
  for (unsigned n = 0; n <= 40; ++n)
    std::printf("n=%2u : 0x%08X\n", n, XpowMod(n));

  const u32 K0_128 = SolveLaneK(128, 0);
  const u32 K1_128 = SolveLaneK(128, 1);
  const u32 K0_512 = SolveLaneK(512, 0);
  const u32 K1_512 = SolveLaneK(512, 1);

  std::puts("\nConstants:");
  std::printf("SingleK_lo = 0x%08X\n", K0_128);
  std::printf("SingleK_hi = 0x%08X\n", K1_128);
  std::printf("FourK_lo   = 0x%08X\n", K0_512);
  std::printf("FourK_hi   = 0x%08X\n", K1_512);
  return 0;
}
