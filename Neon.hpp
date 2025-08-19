#pragma once

#include "Int.hpp"

#include <boost/mp11.hpp>

#include <span>
#include <array>
#include <concepts>
#include <type_traits>
#include <initializer_list>

#include <arm_neon.h>

#include <iostream>
#include <iomanip>

using uint128_t = unsigned __int128;

using NeonPolyTypes = boost::mp11::mp_list<
    poly8x8_t, poly16x4_t, poly64x1_t, poly8x16_t, poly16x8_t, poly64x2_t>;
using NeonUintTypes = boost::mp11::mp_list<
    uint8x8_t,  uint16x4_t, uint32x2_t, uint64x1_t,
    uint8x16_t, uint16x8_t, uint32x4_t, uint64x2_t>;
using NeonVectorTypes = boost::mp11::mp_append<NeonPolyTypes, NeonUintTypes>;

template<typename T>
concept NeonRegister =
      boost::mp11::mp_contains<NeonVectorTypes, std::remove_cvref_t<T>>::value;
template<typename T>
concept NeonPoly =
      boost::mp11::mp_contains<NeonPolyTypes, std::remove_cvref_t<T>>::value;

template<std::integral T, std::size_t L, int C>
struct NeonBase {
  using scalar_type = T;
  static constexpr auto Lanes = L;
  static constexpr auto Code  = C;
}; // NeonBase

template<NeonRegister T> struct NeonTraits;

template<> struct NeonTraits<poly8x8_t > : NeonBase<poly8_t,       8,  4> { };
template<> struct NeonTraits<poly16x4_t> : NeonBase<poly16_t,      4,  5> { };
template<> struct NeonTraits<poly64x1_t> : NeonBase<poly64_t,      1,  6> { };
template<> struct NeonTraits<poly8x16_t> : NeonBase<poly8_t,      16, 36> { };
template<> struct NeonTraits<poly16x8_t> : NeonBase<poly16_t,      8, 37> { };
template<> struct NeonTraits<poly64x2_t> : NeonBase<poly64_t,      2, 38> { };
template<> struct NeonTraits<uint8x8_t > : NeonBase<std::uint8_t,  8, 16> { };
template<> struct NeonTraits<uint16x4_t> : NeonBase<std::uint16_t, 4, 17> { };
template<> struct NeonTraits<uint32x2_t> : NeonBase<std::uint32_t, 1, 18> { };
template<> struct NeonTraits<uint64x1_t> : NeonBase<std::uint64_t, 1, 19> { };
template<> struct NeonTraits<uint8x16_t> : NeonBase<std::uint8_t ,16, 48> { };
template<> struct NeonTraits<uint16x8_t> : NeonBase<std::uint16_t, 8, 49> { };
template<> struct NeonTraits<uint32x4_t> : NeonBase<std::uint32_t, 4, 50> { };
template<> struct NeonTraits<uint64x2_t> : NeonBase<std::uint64_t, 2, 51> { };

template<NeonRegister R>
using ScalarT = NeonTraits<R>::scalar_type;

template<NeonRegister R>
constexpr std::size_t LanesV = NeonTraits<R>::Lanes;

template<NeonRegister R>
constexpr std::size_t CodeV = NeonTraits<R>::Code;

template<NeonRegister T>
requires (sizeof(T) == 16)
constexpr T ByteSwap(T r) noexcept {
  if constexpr (LanesV<T> == 16)
    return r;
  auto b = static_cast<uint8x16_t>(r);
  switch (sizeof(T)/LanesV<T>) {
    case 1: break;
    case 2: b = __builtin_shufflevector(b, b, 1, 0,  3,  2,  5,  4,  7,  6,
                                              9, 8, 11, 10, 13, 12, 15, 14);
            break;
    case 4: b = __builtin_shufflevector(b, b,  3,  2, 1, 0,  7,  6,  5,  4,
                                              11, 10, 9, 8, 15, 14, 13, 12);
            break;
    case 8: b = __builtin_shufflevector(b, b,  7,  6,  5,  4,  3,  2, 1, 0,
                                              15, 14, 13, 12, 11, 10, 9, 8);
            break;
    case 16:b = __builtin_shufflevector(b, b, 15, 14, 13, 12, 11, 10, 9, 8,
                                               7,  6,  5,  4,  3,  2, 1, 0);
            break;
  }
  return static_cast<T>(b);
} // ByteSwap

template<NeonRegister T>
requires (sizeof(T) == 8)
constexpr T ByteSwap(T r) noexcept {
  if constexpr (LanesV<T> == 8)
    return r;
  auto b = static_cast<uint8x8_t>(r);
  switch (sizeof(T)/LanesV<T>) {
    case 1: break;
    case 2: b = __builtin_shufflevector(b, b,  1, 0, 3, 2, 5, 4, 7, 6);
            break;
    case 4: b = __builtin_shufflevector(b, b,  3, 2, 1, 0, 7, 6, 5, 4);
            break;
    case 8: b = __builtin_shufflevector(b, b,  7, 6, 5, 4, 3, 2, 1, 0);
            break;
  }
  return static_cast<T>(b);
} // ByteSwap

template<NeonRegister T>
constexpr T Reverse(T r) noexcept {
  switch (LanesV<T>) {
    case 16: r = __builtin_shufflevector(r, r, 15, 14, 13, 12, 11, 10, 9, 8,
                                                7,  6,  5,  4,  3,  2, 1, 0);
             break;
    case  8: r = __builtin_shufflevector(r, r, 7, 6, 5, 4, 3, 2, 1, 0);
             break;
    case  4: r = __builtin_shufflevector(r, r, 3, 2, 1, 0);
             break;
    case  2: r = __builtin_shufflevector(r, r, 1, 0);
             break;
    case  1: break;
  }
  return r;
} // Reverse

template<NeonRegister T>
constexpr T FullSwap(T r) noexcept {
  if constexpr (sizeof(T) == 16) {
    auto b = static_cast<uint8x16_t>(r);
    b = __builtin_shufflevector(b, b, 15, 14, 13, 12, 11, 10, 9, 8,
                                       7,  6,  5,  4,  3,  2, 1, 0);
    return static_cast<T>(b);
  } else {
    auto b = static_cast<uint8x8_t>(r);
    b = __builtin_shufflevector(b, b, 7,  6,  5,  4,  3,  2, 1, 0);
    return static_cast<T>(b);
  }
} // FullSwap

namespace tjg {

constexpr uint128_t clmul(std::uint64_t x, std::uint64_t y) noexcept{
  return __builtin_neon_vmull_p64(x, y);
}

} // tjg

// This Interface takes integer arguments templated on endian type because I
// believe that letting the Neon do the byteswap is more efficient than using
// std::byteswap(), i.e. letting tjg::Int do it.
template<NeonRegister Reg>
struct NeonV {
  // Assume that Neon is little-endian (a bird told me).
  static constexpr auto Endian = std::endian::little;
  using register_type = Reg;
  using native_type = ScalarT<register_type>;
  using element_type = tjg::Int<native_type, Endian>;
  static constexpr std::size_t N = sizeof(register_type) / sizeof(native_type);
  static constexpr std::size_t size() noexcept { return N; }
  using value_type = std::array<element_type, N>;

  template<std::endian E>
  using Native = tjg::Int<native_type, E>;

  template<std::endian E>
  using NativeSpan = std::span<Native<E>, N>;

  template<std::endian E>
  using ConstNativeSpan = std::span<const Native<E>, N>;

  template<std::endian E>
  using NativeArray = std::array<Native<E>, N>;

  template<std::endian E>
  using ConstNativeArray = std::array<const Native<E>, N>;

  using U8 = tjg::Int<std::uint8_t>;

  template<std::endian E>
  using U64 = tjg::Int<std::uint64_t, E>;

  template<std::endian E>
  using U128 = tjg::Int<uint128_t, E>;

  register_type r;

  template<NeonRegister R, std::endian Rep>
  static constexpr R Load(
            std::span<const tjg::Int<ScalarT<R>, Rep>, LanesV<R>> src) noexcept
  {
    auto p = reinterpret_cast<const ScalarT<R>*>(src.data());
    R tmp;
    if constexpr (sizeof(R) == 16)
      tmp = __builtin_neon_vld1q_v(p, CodeV<R>);
    else
      tmp = __builtin_neon_vld1_v(p, CodeV<R>);
    if (Rep != Endian)
      tmp = ByteSwap(tmp);
    return tmp;
  }

  template<NeonRegister R, std::endian Rep>
  static constexpr void Store(
              std::span<tjg::Int<ScalarT<R>, Rep>, LanesV<R>> dst, R x) noexcept
  {
    auto p = reinterpret_cast<ScalarT<R>*>(dst.data());
    if (Rep != Endian)
      x = ByteSwap(x);
    if constexpr (sizeof(r) == 16)
      __builtin_neon_vst1q_v(p, x, CodeV<R>);
    else
      __builtin_neon_vst1_v(p, x, CodeV<R>);
  }

  template<std::endian Rep>
  constexpr void load(ConstNativeSpan<Rep> src) noexcept
    { r = Load<register_type>(src); }

  template<std::endian Rep>
  constexpr void store(NativeSpan<Rep> dst) noexcept
    { Store(dst, r); }

  constexpr void set(register_type value_) noexcept { r = value_; }

  template<std::endian Rep>
  requires (sizeof(register_type) == sizeof(std::uint64_t))
  constexpr void set(U64<Rep> lo) noexcept
    { r = static_cast<register_type>(Load<uint64x1_t>(std::span{&lo, 1})); }

  template<std::endian Rep>
  constexpr void set(U64<Rep> lo, U64<Rep> hi = U64<Rep>{}) noexcept
    requires (sizeof(register_type) == sizeof(uint128_t))
  {
    uint64x2_t tmp;
    tmp[0] = lo.raw();
    tmp[1] = hi.raw();
    if constexpr (Rep != Endian)
      tmp = ByteSwap(tmp);
    r = static_cast<register_type>(tmp);
  }

  template<std::endian Rep>
  requires (sizeof(register_type) == sizeof(uint128_t))
  constexpr void set(U128<Rep> x) noexcept {
    auto p = reinterpret_cast<const U8*>(&x);
    auto tmp = Load<uint8x16_t>(
                  std::span<const U8, sizeof(uint128_t)>{p, sizeof(uint128_t)});
    if constexpr (Rep != Endian)
      tmp = FullSwap(tmp);
    r = static_cast<register_type>(tmp);
  }

  template<std::endian Rep>
  constexpr void set(ConstNativeSpan<Rep> src) noexcept { load(src); }

  constexpr NeonV& operator=(const NeonV& x) noexcept { set(x.r); return *this; }

  constexpr NeonV& operator=(register_type x) noexcept { set(x); return *this; }

  template<std::endian Rep>
  requires (sizeof(register_type) == sizeof(std::uint64_t))
  constexpr NeonV& operator=(U64<Rep> lo) noexcept
    { set(lo); return *this; }

  template<std::endian Rep>
  requires (sizeof(register_type) == sizeof(uint128_t))
  constexpr NeonV& operator=(U128<Rep> x) noexcept
    { set(x); return *this; }

  template<std::endian Rep>
  constexpr NeonV& operator=(ConstNativeSpan<Rep> src)
    noexcept
    { set(src); return *this; }

  constexpr NeonV() noexcept : r{} { }
  constexpr explicit NeonV(register_type value_) noexcept : r{value_} { }

  template<std::endian Rep>
  requires (sizeof(register_type) == sizeof(std::uint64_t))
  constexpr explicit NeonV(U64<Rep> lo) noexcept
    { set(lo); }

  template<std::endian Rep>
  requires (sizeof(register_type) == 2 * sizeof(std::uint64_t))
  constexpr explicit NeonV(U64<Rep> lo, U64<Rep> hi=0) noexcept
    { set(lo, hi); }

  template<std::endian Rep>
  requires (sizeof(register_type) == sizeof(uint128_t))
  constexpr explicit NeonV(U128<Rep> x) noexcept { set(x); }

  template<std::endian Rep>
  constexpr explicit NeonV(ConstNativeSpan<Rep> src) noexcept
    { set(src); }

  constexpr value_type get() const noexcept {
    value_type rval;
    store(rval);
    return rval;
  }

  template<std::endian Rep>
  requires (sizeof(register_type) == sizeof(uint128_t))
  constexpr void get(U128<Rep>* result) const noexcept {
    static constexpr auto Size = sizeof(register_type);
    auto p = reinterpret_cast<U8*>(result);
    if constexpr (Rep == Endian) {
      Store<uint8x16_t>(std::span<U8, Size>{p, Size}, r);
    } else {
      auto tmp = static_cast<uint8x16_t>(FullSwap(r));
      Store<uint8x16_t>(std::span<U8, Size>{p, Size}, tmp);
    }
  }

  template<std::endian Rep>
  requires (sizeof(register_type) == sizeof(uint128_t))
  constexpr operator U128<Rep>() const noexcept
    { U128<Rep> x; get(&x); return x; }

  template<std::endian Rep>
  requires (sizeof(register_type) == sizeof(std::uint64_t))
  constexpr U64<Rep>& get(U64<Rep>* result) const noexcept {
    static constexpr auto Size = sizeof(register_type);
    auto p = reinterpret_cast<U8*>(result);
    if constexpr (Rep == Endian) {
      Store<uint8x8_t>(std::span<U8, Size>{p, Size}, r);
    } else {
      auto tmp = static_cast<uint8x16_t>(FullSwap(r));
      Store<uint8x8_t>(std::span<U8, Size>{p, Size}, tmp);
    }
  }

  template<std::endian Rep>
  requires (sizeof(register_type) == sizeof(std::uint64_t))
  constexpr operator U64<Rep>() const noexcept
    { U64<Rep> x; get(&x); return x; }

  constexpr element_type get(int idx) const noexcept
    { return element_type{r[idx]}; }

  template<std::endian Rep>
  constexpr void set(int idx, Native<Rep> x) noexcept
    { r[idx] = x.little(); };

  struct Proxy {
    NeonV& self;
    int i;
    constexpr element_type value() const noexcept { return self.get(i); }
    constexpr operator typename element_type::value_type() const noexcept
      { return value().value(); }
    constexpr operator element_type() const noexcept { return value(); }
    constexpr Proxy& operator=(const element_type& v) noexcept
      { self.set(i, v); return *this; }
  }; // Proxy

  Proxy operator[](int i) & { return Proxy{*this, i}; }
  element_type operator[](int i) const & { return get(i); }

  element_type operator[](int i) && { return get(i); }

#ifndef TJG_INT_ONLY
  constexpr void set(std::uint64_t x) noexcept
    requires (sizeof(register_type) == sizeof(std::uint64_t))
    { set(tjg::Int{x}); }

  constexpr void set(std::uint64_t lo, std::uint64_t hi = 0) noexcept
    requires (sizeof(register_type) == sizeof(uint128_t))
    { set(tjg::Int{lo}, tjg::Int{hi}); }

  constexpr void set(uint128_t x) noexcept
    requires (sizeof(register_type) == sizeof(uint128_t))
    { set(tjg::Int{x}); }

  constexpr void set(std::span<const native_type, N> src) noexcept {
    using U = Native<std::endian::native>;
    auto p = reinterpret_cast<const U*>(src.data());
    load(std::span<const U, N>{p, N});
  }

  constexpr NeonV& operator=(std::uint64_t lo)
    noexcept requires(sizeof(register_type) == sizeof(std::uint64_t))
    { set(tjg::Int{lo}); return *this; }

  constexpr NeonV& operator=(uint128_t x) noexcept
    requires (sizeof(register_type) == sizeof(uint128_t))
    { set(tjg::Int{x}); return *this; }

  constexpr NeonV& operator=(std::span<const native_type, N> src) noexcept
    { set(src); return *this; }

  constexpr explicit NeonV(std::uint64_t lo) noexcept
    requires (sizeof(register_type) == sizeof(std::uint64_t))
    { set(tjg::Int{lo}); }

  constexpr explicit NeonV(std::uint64_t lo, std::uint64_t hi=0) noexcept
    requires (sizeof(register_type) == 2 * sizeof(std::uint64_t))
    { set(tjg::Int{lo}, tjg::Int{hi}); }

  constexpr explicit NeonV(uint128_t x) noexcept
    requires (sizeof(register_type) == sizeof(uint128_t)) { set(tjg::Int{x}); }

  constexpr explicit NeonV(std::span<const native_type, N> src) noexcept
    { set(src); }

  constexpr std::array<native_type, N>&
  get(std::array<native_type, N>& dst) const noexcept {
    using U = Native<std::endian::native>;
    auto p = reinterpret_cast<native_type*>(dst.data());
    store(std::span<U, N>{p});
    return dst;
  }

  constexpr void get(uint128_t* result) const noexcept
    requires (sizeof(register_type) == sizeof(uint128_t))
  {
    using U = U128<std::endian::native>;
    get(reinterpret_cast<U*>(result));
  }

  constexpr uint128_t value() const noexcept
    requires (sizeof(register_type) == sizeof(uint128_t))
    {  U128<std::endian::native> tmp; return get(&tmp).value(); }

  constexpr operator uint128_t() const noexcept
    requires (sizeof(register_type) == sizeof(uint128_t))
    { return value(); }

  constexpr std::uint64_t value() const noexcept
    requires (sizeof(register_type) == sizeof(std::uint64_t))
    {  U64<std::endian::native> tmp; return get(&tmp).value(); }

  constexpr operator std::uint64_t() const noexcept
    requires (sizeof(register_type) == sizeof(std::uint64_t))
    { return value(); }

  constexpr native_type& get(int idx, native_type& dst) const noexcept
    { return dst = get(idx).value(); }

  constexpr void set(int idx, native_type x) noexcept
    { set(idx, tjg::Int{x}); }
#endif

  constexpr NeonV operator^(NeonV rhs) const noexcept { return NeonV{r ^ rhs.r}; }
  constexpr NeonV& operator^=(NeonV rhs) noexcept { r ^= rhs.r; return *this; }
}; // NeonV

constexpr NeonV<uint64x2_t> ClMult(NeonV<uint64x2_t> x, NeonV<poly64x2_t> y)
  noexcept
{
  auto p1 = tjg::clmul(x[0].value(), y[0].value());
  auto p2 = tjg::clmul(x[1].value(), y[1].value());
  return NeonV<uint64x2_t>(p1 ^ p2);
}

template<typename T>
struct P: std::conditional<(sizeof(T) > 1), T, unsigned> { };
template<typename T> using P_t = P<T>::type;

template<NeonRegister R>
void PrintRegister(R v) {
  using namespace std;
  using T = ScalarT<R>;
  constexpr auto N = LanesV<R>;
  constexpr auto w = 2 * sizeof(T);
  for (int i = 0; i != N; ++i)
    cout << setfill(' ') << setw(4) << i
         << " 0x" << hex << setfill('0') << setw(w) << P_t<T>{v[i]} << dec
         << endl;
} // PrintRegister
