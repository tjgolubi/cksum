// gen_crc32_lsb_constants_bit_reduce.cpp
// Derive LSB-first fold constants (SingleK/FourK) for CRC-32 using nessan/bit.
// Uses polynomial::reduce(N, /*N_is_exponent=*/true) for x^N mod P* and a
// single 1024x32 Gauss solve per lane.
//
// Build (example):
//   g++ -std=c++20 -O2 gen_crc32_lsb_constants_bit_reduce.cpp -I . -o gen && ./gen

#include "Reflect.hpp"
#include "../ClMul/ClMulNeon.hpp"

#include <bit/vector.h>
#include <bit/matrix.h>
#include <bit/gauss.h>
#include <bit/polynomial.h>

#include <cstdint>
#include <array>
#include <bit>
#include <iostream>
#include <iomanip>
#include <cstdio>

using u32  = std::uint32_t;
using u64  = std::uint64_t;

// LSB-first reciprocal polynomial for CRC-32:
//   P*(x) = x^32 + 0xEDB88320
static constexpr u64 PSTAR = (1ull << 32) | 0xEDB88320ull;

// Build modulus polynomial from integer bits (LSB-first)
static inline bit::polynomial<> make_modulus() {
  bit::vector<> v = bit::vector<>::from(PSTAR);
  return bit::polynomial<>{v};
}

// x^n mod P*(x) → 32-bit residue (LSB-first) using polynomial::reduce
static inline u32 XpowMod(unsigned n, const bit::polynomial<>& Pstar) {
  // IMPORTANT: second argument = true → interpret n as exponent of x
  //auto r = Pstar.reduce(n, /*N_is_exponent=*/true);
  auto r = Pstar.reduce(n, /*N_is_exponent=*/false);
  u32 out = 0;
  r.coefficients().export_bits(out);
  return out;  // degree < 32
}

// Residue of fold-by-N on a one-hot 128-bit input (lo,hi)
static inline u32 TargetResidue(unsigned N, u64 lo, u64 hi, const bit::polynomial<>& Pstar) {
  std::array<u32,128> xp{};
  for (unsigned k = 0; k < 128; ++k) xp[k] = XpowMod(N + k, Pstar);

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
static u32 SolveLaneK(unsigned N, int lane_index, const bit::polynomial<>& Pstar) {
  bit::matrix<> M(32 * 32, 32);
  bit::vector<> b(32 * 32);

  for (u32 i = 0; i < 32; ++i) {
    // Desired residue for basis input in this lane under fold-by-N
    u64 lo = 0, hi = 0;
    if (lane_index == 0) lo = (1ull << i); else hi = (1ull << i);
    const u32 v_i = TargetResidue(N, lo, hi, Pstar);

    // Fill the 32 rows for this i: row (i*32 + t)
    for (u32 t = 0; t < 32; ++t) {
      const u32 row = i * 32 + t;
      // Columns j come from x^(i+j) mod P*
      for (u32 j = 0; j < 32; ++j) {
        if ((XpowMod(i + j, Pstar) >> t) & 1u)
          M.set(row, j);
      }
      if ((v_i >> t) & 1u) b.set(row);
    }
  }

  bit::gauss solver(M, b);
  const auto Kvec = solver(0);   // one solution

  // Pack the 32-bit solution vector into a word
  u32 K = 0;
  for (u32 j = 0; j < 32; ++j)
    if (Kvec[j]) K |= (1u << j);
  return K;
}

using uint128_t = unsigned __int128;
std::ostream& operator<<(std::ostream& os, uint128_t x) {
  using namespace std;
  char c = os.fill('0');
  auto flags = os.flags();
  os << hex << "0x" << setw(16) << static_cast<uint64_t>(x >> 64)
     << setw(16) << static_cast<uint64_t>(x);
  os.fill(c);
  os.flags(flags);
  return os;
} // << uint128_t

int main() {
  const auto Pstar = make_modulus();

  // Sanity: x^n mod P* for n=0..40 (n<32 must equal 1<<n)
  std::puts("Sanity: x^n mod P* for n=0..40");
  for (unsigned n = 0; n <= 40; ++n)
    std::printf("n=%2u : 0x%08X\n", n, XpowMod(n, Pstar));

  // Fold-by-128 and fold-by-512 constants per lane
  const u32 K0_128 = SolveLaneK(128, 0, Pstar);
  const u32 K1_128 = SolveLaneK(128, 1, Pstar);
  const u32 K0_512 = SolveLaneK(512, 0, Pstar);
  const u32 K1_512 = SolveLaneK(512, 1, Pstar);

  std::puts("\nConstants:");
  std::printf("SingleK_lo = 0x%08X\n", K0_128);
  std::printf("SingleK_hi = 0x%08X\n", K1_128);
  std::printf("FourK_lo   = 0x%08X\n", K0_512);
  std::printf("FourK_hi   = 0x%08X\n", K1_512);

  using u128 = unsigned __int128;

  const u64 x = 35;
  const u64 y =  3;
  const u64 z = 17;

  using tjg::IntMath::Reflect;
  auto r1 = neon::ClMul(x, y);
  auto r2 = neon::ClMul(Reflect(x), Reflect(y));
  std::cout << "x=" << x << " y=" << y << " z=" << z
    << " r1=" << r1 << " r2=" << r2 << " (" << (Reflect(r2)>>1) << ")\n";

  r1 = neon::ClMul(r1, z);
  r2 = neon::ClMul(u64(r2>>63), Reflect(z));

  std::cout << " r1=" << r1 << " r2=" << r2 << " (" << (Reflect(r2)>>1) << ")\n";


  return 0;
} // main
