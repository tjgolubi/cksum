#pragma once
#include "Int.hpp"
#include "../tjg32/tjg/Integer.hpp"
#include <bit>
#include <type_traits>
#include <cstdint>

namespace tjg::crc {

template<int _Bits_, std::uint64_t _PolyNoTop_>
requires (_Bits_ >= 3 && _Bits_ <= 64)
struct CrcConsts {
public:
  static constexpr int Bits = _Bits_;
  static constexpr std::uint64_t Poly = _PolyNoTop_;

  static constexpr std::uint64_t Mask = ~std::uint64_t{0} >> (64-Bits);
  static_assert(((Poly & ~Mask) == 0), "Polynomial out of range");

private:
  template<int Exp>
  static constexpr std::uint64_t XpowMod() noexcept {
    if constexpr (Exp < Bits) {
      return std::uint64_t{1} << Exp;
    } else {
      constexpr std::uint64_t TopBit = std::uint64_t{1} << (Bits-1);
      std::uint64_t v = Poly;
      for (int i = Bits; i < Exp; ++i) {
        const bool carry = ((v & TopBit) != 0);
        v <<= 1;
        if (carry) v ^= Poly;
      }
      return (v & Mask);
    }
  } // XpowMod

  static constexpr std::uint64_t ComputeMu() noexcept {
    using uint128_t = unsigned __int128;
    constexpr uint128_t P = (uint128_t{1} << Bits) | uint128_t{Poly};
    constexpr uint128_t X2n =
                    (Bits == 64) ? uint128_t{0} : (uint128_t{1} << (2 * Bits));

    uint128_t r = X2n ^ (P << Bits);
    std::uint64_t mu = (Bits==64) ? 0 : (std::uint64_t{1} << Bits);

    for (;;) {
      int degree = std::bit_width(r);
      if (degree <= Bits)
        break;
      const int s = (degree - 1) - Bits;
      mu |= std::uint64_t{1} << s;
      r  ^= P << s;
    }
    return mu;
  } // ComputeMu

public:
  static constexpr std::uint64_t K128_lo = XpowMod<1*128>();
  static constexpr std::uint64_t K128_hi = XpowMod<1*128 + 64>();
  static constexpr std::uint64_t K512_lo = XpowMod<4*128>();
  static constexpr std::uint64_t K512_hi = XpowMod<4*128 + 64>();
  static constexpr std::uint64_t Mu2N    = ComputeMu();
}; // CrcConsts

using Crc32Consts = CrcConsts<32, 0x04c11db7>;

} // tjg::crc
