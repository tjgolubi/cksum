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
#include <bit>

#if __has_include(<arm_neon.h>)
#include <arm_neon.h>
#define SIMD_NEON 1
#elif __has_include(<immintrin.h>)
#include <immintrin.h>
#define SIMD_SSE 1
#else
#include "../ClMul/ClMul.hpp"
#endif

#include <iostream>
#include <iomanip>

using uint128_t = unsigned __int128;

namespace simd {

#if defined(SIMD_NEON)
using uint8x16_t = [[clang::neon_vector_type(16)]] std::uint8_t;
using uint16x8_t = [[clang::neon_vector_type( 8)]] std::uint16_t;
using uint32x4_t = [[clang::neon_vector_type( 4)]] std::uint32_t;
using uint64x2_t = [[clang::neon_vector_type( 2)]] std::uint64_t;
#else
template<std::unsigned_integral T>
using VectorT [[gnu::vector_size(16)]] = T;
using uint8x16_t = VectorT<std::uint8_t >;
using uint16x8_t = VectorT<std::uint16_t>;
using uint32x4_t = VectorT<std::uint32_t>;
using uint64x2_t = VectorT<std::uint64_t>;
#endif

using VectorTypes = boost::mp11::mp_list<
                                uint8x16_t, uint16x8_t, uint32x4_t, uint64x2_t>;

template<typename V>
concept VectorType =
          boost::mp11::mp_contains<VectorTypes, std::remove_cvref_t<V>>::value;

template<VectorType V> struct ScalarType;
template<> struct ScalarType<uint8x16_t> { using type = std::uint8_t ; };
template<> struct ScalarType<uint16x8_t> { using type = std::uint16_t; };
template<> struct ScalarType<uint32x4_t> { using type = std::uint32_t; };
template<> struct ScalarType<uint64x2_t> { using type = std::uint64_t; };
template<VectorType V> using ScalarT = ScalarType<V>::type;

template<VectorType V>
constexpr std::size_t LanesV = sizeof(V)/sizeof(ScalarT<V>);

template<VectorType V>
constexpr V ByteSwap(V v) noexcept {
  if constexpr (LanesV<V> == 16)
    return v;
  auto b = std::bit_cast<uint8x16_t>(v);
  switch (sizeof(ScalarT<V>)) {
    case 1: break;
    case 2: b = __builtin_shufflevector(b, b, 1, 0,  3,  2,  5,  4,  7,  6,
                                              9, 8, 11, 10, 13, 12, 15, 14);
            break;
    case 4: b = __builtin_shufflevector(b, b, 3,  2, 1, 0,  7,  6,  5,  4,
                                             11, 10, 9, 8, 15, 14, 13, 12);
            break;
    case 8: b = __builtin_shufflevector(b, b,  7,  6,  5,  4,  3,  2, 1, 0,
                                              15, 14, 13, 12, 11, 10, 9, 8);
            break;
    case 16:b = __builtin_shufflevector(b, b, 15, 14, 13, 12, 11, 10, 9, 8,
                                               7,  6,  5,  4,  3,  2, 1, 0);
            break;
  }
  return std::bit_cast<V>(b);
} // ByteSwap

constexpr uint8x16_t Reverse(uint8x16_t v) noexcept {
  return __builtin_shufflevector(v, v, 15, 14, 13, 12, 11, 10, 9, 8,
                                        7,  6,  5,  4,  3,  2, 1, 0);
}

constexpr uint16x8_t Reverse(uint16x8_t v) noexcept
  { return __builtin_shufflevector(v, v, 7, 6, 5, 4, 3, 2, 1, 0); }

constexpr uint32x4_t Reverse(uint32x4_t v) noexcept
  { return __builtin_shufflevector(v, v, 3, 2, 1, 0); }

constexpr uint64x2_t Reverse(uint64x2_t v) noexcept
  { return __builtin_shufflevector(v, v, 1, 0); }

template<VectorType V>
constexpr V FullSwap(V v) noexcept {
  return (V) Reverse((uint8x16_t) v);
} // FullSwap

constexpr uint128_t clmul(std::uint64_t x, std::uint64_t y) noexcept {
#if defined(SIMD_NEON)
  return __builtin_neon_vmull_p64(x, y);
#elif defined(SIMD_SSE)
  auto vx = _mm_set_epi64x(x, y);
  auto vz = _mm_clmulepi64_si128(vx, vx, 0x01);
  return std::bit_cast<uint128_t>(vz);
#else
  return tjg::ClMul(x, y);
#endif
} // clmul

constexpr uint64x2_t ClMulDiag(uint64x2_t x, uint64x2_t y)
  noexcept
{
#if defined(SIMD_SSE)
  auto t1 = _mm_clmulepi64_si128((__m128i) x, (__m128i) y, 0x00);
  auto t2 = _mm_clmulepi64_si128((__m128i) x, (__m128i) y, 0x11);
  return std::bit_cast<uint64x2_t>(t1 ^ t2);
#else
  auto z = clmul(x[0], y[0]) ^ clmul(x[1], y[1]);
  return std::bit_cast<uint64x2_t>(z);
#endif
} // ClMulDiag

// This Interface takes integer arguments templated on endian type because I
// believe that letting the SIMD do the byteswap is more efficient than using
// std::byteswap(), i.e. letting tjg::Int do it.
template<VectorType _V_>
struct Simd {
  using register_type = _V_;
  using native_type  = ScalarT<_V_>;
  static constexpr std::size_t Lanes = LanesV<_V_>;
  static constexpr std::size_t size() noexcept { return Lanes; }
  using full_type = uint128_t;

  register_type r = register_type{};

public:
  constexpr void set(register_type x) noexcept { r = x; }
  constexpr void set(Simd x) noexcept { set(x.r); }
  constexpr void set(full_type x) noexcept
    { r = std::bit_cast<register_type>(x) ; }

  template<std::integral T, std::endian E>
  requires tjg::NonNarrowing<T, full_type>
  constexpr void set(tjg::Int<T, E> x) noexcept {
    r = std::bit_cast<register_type>(x.raw());
    if constexpr (E != std::endian::native)
      r = FullSwap(r);
  }

  constexpr void set_all(native_type x) noexcept { r = x; }

  template<std::integral T, std::endian E>
  requires tjg::NonNarrowing<T, native_type>
  constexpr void set_all(tjg::Int<T, E> x) noexcept {
    r = x.raw();
    if constexpr (E != std::endian::native)
      r = ByteSwap(r);
  }

  constexpr Simd& operator=(register_type x) noexcept { set(x); return *this; }
  constexpr Simd& operator=(const Simd& x) noexcept
    { set(x.r); return *this; }
  constexpr Simd& operator=(native_type x) noexcept { set(x); return *this; }

  template<std::integral T, std::endian E>
  requires tjg::NonNarrowing<T, full_type>
  constexpr Simd& operator=(tjg::Int<T, E> x) noexcept
    { set(x); return *this; }

  constexpr Simd() noexcept : r{} { }
  constexpr explicit Simd(register_type value_) noexcept : r{value_} { }
  constexpr explicit Simd(full_type v) noexcept { set(v); }

  template<std::integral T, std::endian E>
  requires tjg::NonNarrowing<T, full_type>
  constexpr explicit Simd(tjg::Int<T, E> x) noexcept { set(x); }

private:
  //----------------------------------------------------------
  // _set span of values
  //----------------------------------------------------------
  template<std::integral T>
  constexpr void _set(std::span<const T, Lanes> values) noexcept {
    [&]<int... Is>(std::integer_sequence<int, Is...>) {
      ((r[Is] = static_cast<native_type>(values[Is])), ...);
    }(std::make_integer_sequence<int, static_cast<int>(Lanes)>{});
  }

  //----------------------------------------------------------
  // _setInt span of tjg::Int's
  //----------------------------------------------------------
  template<std::integral T, std::endian E>
  constexpr void _setInt(std::span<const tjg::Int<T, E>, Lanes> values) noexcept
  {
    [&]<int... Is>(std::integer_sequence<int, Is...>) {
      ((r[Is] = static_cast<native_type>(values[Is].raw())), ...);
    }(std::make_integer_sequence<int, static_cast<int>(Lanes)>{});
    if constexpr (E != std::endian::native)
      r = ByteSwap(r);
  }

public:
  //----------------------------------------------------------
  // set(...) overloads -- native lane-like
  //----------------------------------------------------------
  template<std::integral T, std::size_t M>
  requires (M == static_cast<std::size_t>(Lanes)
    && tjg::NonNarrowing<T, native_type>)
  constexpr void set(std::span<const T, M> values) noexcept { _set(values); }

  template<std::integral T>
  requires tjg::NonNarrowing<T, native_type>
  constexpr void set(std::span<const T> values) noexcept {
    if (values.size() != static_cast<std::size_t>(Lanes))
      throw std::out_of_range{"Simd::set: span size != Lanes"};
    _set(std::span<const T, Lanes>{values.data(), Lanes});
  }

  template<typename... Args>
  requires (sizeof...(Args) == Lanes
    && (tjg::NonNarrowing<Args, native_type> && ...))
  constexpr void set(Args&&... args) noexcept {
    const native_type tmp[Lanes]
                    { static_cast<native_type>(std::forward<Args>(args))...  };
    _set(std::span<const native_type, Lanes>{tmp, Lanes});
  }

  // initializer_list (native), runtime size check
  template<std::integral T>
  requires tjg::NonNarrowing<T, native_type>
  constexpr void set(std::initializer_list<T> ilist) {
    if (ilist.size() != static_cast<std::size_t>(Lanes))
      throw std::out_of_range{"Simd::set: initializer_list size != Lanes"};
    _set(std::span<const T, Lanes>{ilist.begin(), Lanes});
  }

  //----------------------------------------------------------
  // set(...) overloads -- tjg::Int<T, E> sources
  //----------------------------------------------------------
  template<std::integral T, std::endian E, std::size_t M>
  requires (M == static_cast<std::size_t>(Lanes)
    && tjg::NonNarrowing<T, native_type>)
  constexpr void set(std::span<const tjg::Int<T, E>, M> values) noexcept
    { _setInt(values); }

  template<std::integral T, std::endian E>
  requires tjg::NonNarrowing<T, native_type>
  constexpr void set(std::span<const tjg::Int<T, E>> values) {
    if (values.size() != static_cast<std::size_t>(Lanes))
      throw std::out_of_range{"Simd::set: size != Lanes"};
    _setInt(std::span<const tjg::Int<T, E>, Lanes>{values.data(), Lanes});
  }

  // initializer_list (tjg::Int<T,E>), runtime size check
  template<std::integral T, std::endian E>
  constexpr void set(std::initializer_list<tjg::Int<T, E>> ilist) noexcept {
    if (ilist.size() != static_cast<std::size_t>(Lanes))
      throw std::out_of_range{"Simd::set: initializer_list size != Lanes"};
    _setInt(std::span<const tjg::Int<T, E>, Lanes>{ilist.begin(), Lanes});
  }

  //----------------------------------------------------------
  // operator= forwarding to set(...)
  //----------------------------------------------------------
  template<std::integral T, std::size_t M>
  requires (M == static_cast<std::size_t>(Lanes)
    && tjg::NonNarrowing<T, native_type>)
  constexpr Simd& operator=(std::span<const T, M> values) noexcept
    { set(values); return *this; }

  template<std::integral T>
  requires tjg::NonNarrowing<T, native_type>
  constexpr Simd& operator=(std::span<const T> values) noexcept
    { set(values); return *this; }

  // init-list native
  template<std::integral T>
  requires tjg::NonNarrowing<T, native_type>
  constexpr Simd& operator=(std::initializer_list<T> ilist) noexcept
    { set(ilist); return *this; }

  // spans of tjg::Int<T,E>
  template<std::integral T, std::endian E, std::size_t M>
  requires (M == static_cast<std::size_t>(Lanes)
    && tjg::NonNarrowing<T, native_type>)
  Simd& operator=(std::span<const tjg::Int<T, E>, M> values)
    { set(values); return *this; }

  template<std::integral T, std::endian E>
  requires tjg::NonNarrowing<T, native_type>
  constexpr Simd& operator=(std::span<const tjg::Int<T, E>> values) noexcept
    { set(values); return *this; }

  // init-list tjg::Int<T,E>
  template<std::integral T, std::endian E>
  constexpr Simd& operator=(std::initializer_list<tjg::Int<T, E>> ilist)
    noexcept
    { set(ilist); return *this; }

  //----------------------------------------------------------
  // Constructors (native-only): span + variadic
  //----------------------------------------------------------
  template<std::integral T, std::size_t M>
  requires (M == static_cast<std::size_t>(Lanes)
    && tjg::NonNarrowing<T, native_type>)
  constexpr explicit Simd(std::span<const T, M> s) noexcept { set(s); }

  template<std::integral T>
  requires tjg::NonNarrowing<T, native_type>
  constexpr explicit Simd(std::span<const T> s) { set(s); }

  template<typename... Args>
  requires (sizeof...(Args) == Lanes
    && (tjg::NonNarrowing<Args, native_type> && ...))
  constexpr explicit Simd(Args&&... args) noexcept
    { set(std::forward<Args>(args)...); }

  // --------------------------------------------------------------------------
  //  read access
  // --------------------------------------------------------------------------
  constexpr void get(full_type* result) const noexcept
    { *result = std::bit_cast<full_type>(r); }

  constexpr operator full_type() const noexcept
    { return std::bit_cast<full_type>(r); }

  template<std::endian E>
  constexpr void get(tjg::Int<full_type, E>* result) const noexcept {
    if constexpr (E == std::endian::native)
      result->raw() = std::bit_cast<full_type>(r);
    else
      result->raw() = std::bit_cast<full_type>(FullSwap(r));
  }

  template<std::endian E>
  constexpr operator tjg::Int<full_type, E>() const noexcept {
    tjg::Int<full_type, E> result;
    if constexpr (E == std::endian::native)
      result.raw() = std::bit_cast<full_type>(r);
    else
      result.raw() = std::bit_cast<full_type>(FullSwap(r));
    return result;
  }

  constexpr native_type get(int idx) const noexcept { return r[idx]; }

  constexpr native_type at(int idx) const {
    using namespace std::literals;
    if (idx < 0 || idx >= static_cast<int>(Lanes))
      throw std::out_of_range{"Simd::get("s + std::to_string(idx) + "): "s};
    return get(idx);
  }

  constexpr void set(int idx, native_type x) {
    using namespace std::literals;
    if (idx < 0 || idx >= static_cast<int>(Lanes))
      throw std::out_of_range{"Simd::set("s + std::to_string(idx) + "): "s};
    r[idx] = x;
  }

  struct Ref {
    register_type& r;
    int idx;
    constexpr operator native_type() const { return r[idx]; }
    constexpr Ref& operator=(native_type x) { r[idx] = x; return *this; }
  };

  constexpr Ref         operator[](int i) &       noexcept { return Ref{r, i}; }
  constexpr native_type operator[](int i) const & noexcept { return r[i]; }
  constexpr native_type operator[](int i) &&      noexcept { return r[i]; }

// Native-type helpers.
  template<std::integral T>
    requires tjg::NonNarrowing<native_type, T>
  constexpr void get(std::span<T, Lanes> values) const noexcept {
    [&]<int... Is>(std::integer_sequence<int, Is...>) {
      ((values[Is] = static_cast<T>(r[Is])), ...);
    }(std::make_integer_sequence<int, static_cast<int>(Lanes)>{});
  }

  template<std::integral T>
    requires tjg::NonNarrowing<native_type, T>
  constexpr void get(std::span<T> values) const {
    if (values.size() != Lanes)
      throw std::out_of_range{"Simd::get: span size != Lanes"};
    get(std::span<T, Lanes>{values.data(), Lanes});
  }

  template<std::integral T, std::endian E>
    requires tjg::NonNarrowing<native_type, T>
  constexpr void get(std::span<tjg::Int<T, E>, Lanes> values) const noexcept {
    auto tmp = r;
    if constexpr (E != std::endian::native)
      tmp = ByteSwap(tmp);
    [&]<int... Is>(std::integer_sequence<int, Is...>) {
      ((values[Is].raw() = static_cast<T>(tmp[Is])), ...);
    }(std::make_integer_sequence<int, static_cast<int>(Lanes)>{});
  }

  template<std::integral T, std::endian E>
    requires tjg::NonNarrowing<native_type, T>
  constexpr void get(std::span<tjg::Int<T, E>> values) const {
    if (values.size() != Lanes)
      throw std::out_of_range{"Simd::get: span size != Lanes"};
    get(std::span<tjg::Int<T, E>, Lanes>{values.data(), Lanes});
  }

  template<std::integral T>
  requires tjg::NonNarrowing<native_type, T>
  constexpr void get(std::array<T, Lanes>& dst) const noexcept
    { get(std::span{dst}); }

  template<std::integral T, std::endian E>
  requires tjg::NonNarrowing<native_type, T>
  constexpr void get(std::array<tjg::Int<T, E>, Lanes>& dst) const noexcept
    { get(std::span{dst}); }

  constexpr Simd operator^(Simd rhs) const noexcept
    { return Simd{r ^ rhs.r}; }
  constexpr Simd& operator^=(Simd rhs) noexcept { r ^= rhs.r; return *this; }
  constexpr Simd operator<<(std::integral auto rhs) const noexcept
    { return Simd{r << rhs}; }
  constexpr Simd operator>>(std::integral auto rhs) const noexcept
    { return Simd{r >> rhs}; }
  constexpr Simd& operator<<=(std::integral auto rhs) const noexcept
    { return r <<= rhs; return *this; }
  constexpr Simd& operator>>=(std::integral auto rhs) const noexcept
    { return r >>= rhs; return *this; }
}; // Simd

constexpr Simd<uint64x2_t> ClMulDiag(Simd<uint64x2_t> x, Simd<uint64x2_t> y)
  noexcept
  { return Simd<uint64x2_t>{ClMulDiag(x.r, y.r)}; }

template<std::integral T>
struct P: std::conditional<(sizeof(T) > 1), T, unsigned> { };
template<std::integral T> using P_t = P<T>::type;

template<VectorType V>
void PrintVector(V v) {
  using namespace std;
  using T = ScalarT<V>;
  constexpr auto Lanes = LanesV<V>;
  constexpr auto W = 2 * sizeof(T);
  for (int i = 0; i != Lanes; ++i) {
    cout << setfill(' ') << setw(4) << i
         << " 0x" << hex << setfill('0') << setw(W) << P_t<T>{v[i]} << dec
         << endl;
  }
} // PrintVector

} // simd
