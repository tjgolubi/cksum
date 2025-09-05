#pragma once

#include "Int.hpp"

#include <boost/mp11.hpp>

#include <span>
#include <array>
#include <concepts>
#include <type_traits>
#include <initializer_list>
#include <exception>
#include <string>
#include <utility>

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

constexpr uint128_t clmul(std::uint64_t x, std::uint64_t y) noexcept
  { return __builtin_neon_vmull_p64(x, y); }

} // tjg

// This Interface takes integer arguments templated on endian type because I
// believe that letting the Neon do the byteswap is more efficient than using
// std::byteswap(), i.e. letting tjg::Int do it.
template<NeonRegister Reg>
struct NeonV {
  using register_type = Reg;
  using native_type  = NeonTraits<Reg>::scalar_type;
  static constexpr std::size_t Lanes = NeonTraits<Reg>::Lanes;
  static constexpr std::size_t size() noexcept { return Lanes; }
  using full_type  = std::conditional_t<(sizeof(Reg) == 16),
                                        uint128_t, std::uint64_t>;

  register_type r = register_type{};

public:
  constexpr void set(register_type value_) noexcept { r = value_; }
  constexpr void set(NeonV x) noexcept { set(x.r); }

  constexpr void set(full_type x) noexcept { r = (register_type) x ; }

  template<std::integral U, std::endian E>
  requires tjg::NonNarrowing<U, full_type>
  constexpr void set(tjg::Int<U, E> x) noexcept {
    r = (register_type) x.raw();
    if constexpr (E != std::endian::native)
      r = FullSwap(r);
  }

  constexpr NeonV& operator=(register_type x) noexcept { set(x); return *this; }
  constexpr NeonV& operator=(const NeonV& x) noexcept { set(x.r); return *this; }

  template<std::integral U, std::endian E>
  requires tjg::NonNarrowing<U, full_type>
  constexpr NeonV& operator=(tjg::Int<U, E> x) noexcept
    { set(x); return *this; }

  constexpr NeonV() noexcept : r{} { }
  constexpr explicit NeonV(register_type value_) noexcept : r{value_} { }
  constexpr explicit NeonV(full_type v) noexcept { set(v); }

private:
  //----------------------------------------------------------
  // _set span of values
  //----------------------------------------------------------
  template<std::integral V>
  constexpr void _set(std::span<const V, Lanes> values) noexcept {
    [&]<int... Is>(std::integer_sequence<int, Is...>) {
      ((r[Is] = static_cast<native_type>(values[Is])), ...);
    }(std::make_integer_sequence<int, static_cast<int>(Lanes)>{});
  }

  //----------------------------------------------------------
  // _set span of tjg::Int's
  //----------------------------------------------------------
  template<std::integral V, std::endian E>
  constexpr void _setInt(std::span<const tjg::Int<V, E>, Lanes> values) noexcept
  {
    [&]<int... Is>(std::integer_sequence<int, Is...>) {
      ((r[Is] = static_cast<native_type>(values[Is].raw())), ...);
    }(std::make_integer_sequence<int, static_cast<int>(Lanes)>{});
    if constexpr (E != std::endian::native)
      r = ByteSwap(r);
  }

public:
  //----------------------------------------------------------
  // set(...) overloads — native lane-like
  //----------------------------------------------------------
  template<std::integral V, std::size_t M>
  requires (M == static_cast<std::size_t>(Lanes)
    && tjg::NonNarrowing<V, native_type>)
  constexpr void set(std::span<const V, M> values) noexcept { _set(values); }

  template<std::integral V>
  requires tjg::NonNarrowing<V, native_type>
  constexpr void set(std::span<const V> values) noexcept {
    if (values.size() != static_cast<std::size_t>(Lanes))
      throw std::out_of_range{"NeonV::set: span size != Lanes"};
    _set(std::span<const V, Lanes>{values.data(), Lanes});
  }

  template<class... Args>
  requires (sizeof...(Args) == Lanes
    && (tjg::NonNarrowing<Args, native_type> && ...))
  constexpr void set(Args&&... args) noexcept {
    const native_type tmp[Lanes]
                    { static_cast<native_type>(std::forward<Args>(args))...  };
    _set(std::span<const native_type, Lanes>{tmp, Lanes});
  }

  // initializer_list (native), runtime size check
  template<typename V>
  requires tjg::NonNarrowing<V, native_type>
  constexpr void set(std::initializer_list<V> ilist) {
    if (ilist.size() != static_cast<std::size_t>(Lanes))
      throw std::out_of_range{"NeonV::set: initializer_list size != Lanes"};
    _set(std::span<const V, Lanes>{ilist.begin(), Lanes});
  }

  //----------------------------------------------------------
  // set(...) overloads — tjg::Int<U, E> sources
  //----------------------------------------------------------
  template<typename U, std::endian E, std::size_t M>
  requires (M == static_cast<std::size_t>(Lanes)
    && tjg::NonNarrowing<U, native_type>)
  constexpr void set(std::span<const tjg::Int<U, E>, M> values) noexcept
    { _setInt(values); }

  template<typename U, std::endian E>
  requires tjg::NonNarrowing<U, native_type>
  constexpr void set(std::span<const tjg::Int<U, E>> values) {
    if (values.size() != static_cast<std::size_t>(Lanes))
      throw std::out_of_range{"NeonV::set: size != Lanes"};
    _setInt(std::span<const tjg::Int<U, E>, Lanes>{values.data(), Lanes});
  }

  // initializer_list (tjg::Int<U,E>), runtime size check
  template<typename U, std::endian E>
  constexpr void set(std::initializer_list<tjg::Int<U, E>> ilist) noexcept {
    if (ilist.size() != static_cast<std::size_t>(Lanes))
      throw std::out_of_range{"NeonV::set: initializer_list size != Lanes"};
    _setInt(std::span<const tjg::Int<U, E>, Lanes>{ilist.begin(), Lanes});
  }

  //----------------------------------------------------------
  // operator= forwarding to set(...)
  //----------------------------------------------------------
  template<typename V, std::size_t M>
  requires (M == static_cast<std::size_t>(Lanes)
    && tjg::NonNarrowing<V, native_type>)
  constexpr NeonV& operator=(std::span<const V, M> values) noexcept
    { set(values); return *this; }

  template<typename V>
  requires tjg::NonNarrowing<V, native_type>
  constexpr NeonV& operator=(std::span<const V> values) noexcept
    { set(values); return *this; }

  // init-list native
  template<typename V>
  requires tjg::NonNarrowing<V, native_type>
  constexpr NeonV& operator=(std::initializer_list<V> ilist) noexcept
    { set(ilist); return *this; }

  // spans of tjg::Int<U,E>
  template<typename U, std::endian E, std::size_t M>
  requires (M == static_cast<std::size_t>(Lanes)
    && tjg::NonNarrowing<U, native_type>)
  NeonV& operator=(std::span<const tjg::Int<U, E>, M> values)
    { set(values); return *this; }

  template<typename U, std::endian E>
  requires tjg::NonNarrowing<U, native_type>
  constexpr NeonV& operator=(std::span<const tjg::Int<U, E>> values) noexcept
    { set(values); return *this; }

  // init-list tjg::Int<U,E>
  template<typename U, std::endian E>
  constexpr NeonV& operator=(std::initializer_list<tjg::Int<U, E>> ilist)
    noexcept
    { set(ilist); return *this; }

  //----------------------------------------------------------
  // Constructors (native-only): span + variadic
  //----------------------------------------------------------
  template<typename V, std::size_t M>
  requires (M == static_cast<std::size_t>(Lanes)
    && tjg::NonNarrowing<V, native_type>)
  constexpr explicit NeonV(std::span<const V, M> s) noexcept { set(s); }

  template<typename V>
  requires tjg::NonNarrowing<V, native_type>
  constexpr explicit NeonV(std::span<const V> s) { set(s); }

  template<class... Args>
  requires (sizeof...(Args) == Lanes
    && (tjg::NonNarrowing<Args, native_type> && ...))
  constexpr explicit NeonV(Args&&... args) noexcept
    { set(std::forward<Args>(args)...); }

  // --------------------------------------------------------------------------
  //  read access
  // --------------------------------------------------------------------------
  constexpr void get(full_type* result) const noexcept
    { *result = (full_type) r; }

  constexpr operator full_type() const noexcept
    { return (full_type) r; }

  template<std::endian E>
  constexpr void get(tjg::Int<full_type, E>* result) const noexcept {
    if constexpr (E == std::endian::native)
      result->raw() = (full_type) r;
    else
      result->raw() = (full_type) FullSwap(r);
  }

  template<std::endian E>
  constexpr operator tjg::Int<full_type, E>() const noexcept {
    tjg::Int<full_type, E> result;
    if constexpr (E == std::endian::native)
      result.raw() = (full_type) r;
    else
      result.raw() = (full_type) FullSwap(r);
    return result;
  }

  constexpr native_type get(int idx) const {
    using namespace std::literals;
    if (idx < 0 || idx >= static_cast<int>(Lanes))
      throw std::out_of_range{"NeonV::get("s + std::to_string(idx) + "): "s};
    return r[idx];
  }

  constexpr void set(int idx, native_type x) {
    using namespace std::literals;
    if (idx < 0 || idx >= static_cast<int>(Lanes))
      throw std::out_of_range{"NeonV::set("s + std::to_string(idx) + "): "s};
    r[idx] = x;
  }

  struct Proxy {
    NeonV& self;
    int i;
    constexpr operator native_type() const { return self.get(i); }
    constexpr Proxy& operator=(native_type x) { self.set(i, x); return *this; }
  }; // Proxy

  Proxy operator[](int i) & { return Proxy{*this, i}; }
  native_type operator[](int i) const & { return get(i); }

  native_type operator[](int i) && { return get(i); }

// Native-type helpers.
  template<typename V>
    requires tjg::NonNarrowing<native_type, V>
  constexpr void get(std::span<V, Lanes> values) const noexcept {
    [&]<int... Is>(std::integer_sequence<int, Is...>) {
      ((values[Is] = static_cast<V>(r[Is])), ...);
    }(std::make_integer_sequence<int, static_cast<int>(Lanes)>{});
  }

  template<typename V>
    requires tjg::NonNarrowing<native_type, V>
  constexpr void get(std::span<V> values) const {
    if (values.size() != Lanes)
      throw std::out_of_range{"NeonV::get: span size != Lanes"};
    get(std::span<V, Lanes>{values.data(), Lanes});
  }

  template<typename U, std::endian E>
    requires tjg::NonNarrowing<native_type, U>
  constexpr void get(std::span<tjg::Int<U, E>, Lanes> values) const noexcept {
    auto tmp = r;
    if constexpr (E != std::endian::native)
      tmp = ByteSwap(tmp);
    [&]<int... Is>(std::integer_sequence<int, Is...>) {
      ((values[Is].raw() = static_cast<U>(tmp[Is])), ...);
    }(std::make_integer_sequence<int, static_cast<int>(Lanes)>{});
  }

  template<typename U, std::endian E>
    requires tjg::NonNarrowing<native_type, U>
  constexpr void get(std::span<tjg::Int<U, E>> values) const {
    if (values.size() != Lanes)
      throw std::out_of_range{"NeonV::get: span size != Lanes"};
    get(std::span<tjg::Int<U, E>, Lanes>{values.data(), Lanes});
  }

  template<typename V>
  requires tjg::NonNarrowing<native_type, V>
  void get(std::array<V, Lanes>& dst) const noexcept { get(std::span{dst}); }

  template<typename U, std::endian E>
  requires tjg::NonNarrowing<native_type, U>
  void get(std::array<tjg::Int<U, E>, Lanes>& dst) const noexcept
    { get(std::span{dst}); }

  constexpr NeonV operator^(NeonV rhs) const noexcept { return NeonV{r ^ rhs.r}; }
  constexpr NeonV& operator^=(NeonV rhs) noexcept { r ^= rhs.r; return *this; }
}; // NeonV

constexpr NeonV<uint64x2_t> ClMult(NeonV<uint64x2_t> x, NeonV<poly64x2_t> y)
  noexcept
{
  auto p1 = tjg::clmul(x[0], y[0]);
  auto p2 = tjg::clmul(x[1], y[1]);
  return NeonV<uint64x2_t>(p1 ^ p2);
}

template<typename T>
struct P: std::conditional<(sizeof(T) > 1), T, unsigned> { };
template<typename T> using P_t = P<T>::type;

template<NeonRegister R>
void PrintRegister(R v) {
  using namespace std;
  using T = ScalarT<R>;
  constexpr auto Lanes = LanesV<R>;
  constexpr auto w = 2 * sizeof(T);
  for (int i = 0; i != Lanes; ++i)
    cout << setfill(' ') << setw(4) << i
         << " 0x" << hex << setfill('0') << setw(w) << P_t<T>{v[i]} << dec
         << endl;
} // PrintRegister
