#pragma once
#include <concepts>   // std::integral
#include <functional> // std::hash
#include <type_traits>// std::is_trivially_copyable_t
#include <compare>    // operator<=>
#include <bit>        // std::endian, std::byteswap
#include <memory>     // std::addressof
#include <cstddef>    // std::size_t

namespace tjg {

constexpr std::endian operator~(std::endian x) noexcept {
  if (x == std::endian::little)
    return std::endian::big;
  else
    return std::endian::little;
}

template<std::integral T=int, std::endian E = std::endian::native>
requires (std::same_as<T, std::remove_cv_t<T>>
       && !std::is_reference_v<T>)
class Int {
public:
  using value_type = T;
  static constexpr std::endian Endian = E;
  using SwapT   = Int<T, ~Endian>;
  using NativeT = Int<T, std::endian::native>;

private:
  value_type _raw;

  template<std::endian Rep>
  constexpr T _get() const noexcept {
    if constexpr (E == Rep)
      return _raw;
    else
      return std::byteswap(_raw);
  }
  constexpr void _set(T x) noexcept {
    if constexpr (E == std::endian::native)
      _raw = x;
    else
      _raw = std::byteswap(x);
  }

public:
  constexpr Int() noexcept = default;
  constexpr Int(const Int&) noexcept = default;
  constexpr Int& operator=(const Int&) = default;
  constexpr bool operator==(const Int&) const = default;

  constexpr explicit(E != std::endian::native) Int(T x) noexcept { _set(x); }
  constexpr Int& operator=(T x) noexcept { _set(x); return *this; }
  constexpr       T& raw()       noexcept { return _raw;   }
  constexpr const T& raw() const noexcept { return _raw;   }

  constexpr       T* ptr()       noexcept requires (E == std::endian::native)
    { return std::addressof(_raw); }

  constexpr const T* ptr() const noexcept requires (E == std::endian::native)
    { return std::addressof(_raw); }

  [[nodiscard]] constexpr T native()   const noexcept
    { return _get<std::endian::native>(); }

  [[nodiscard]] constexpr T little()   const noexcept
    { return _get<std::endian::little>(); }

  [[nodiscard]] constexpr T big()      const noexcept
    { return _get<std::endian::big>(); }

  constexpr operator T() const noexcept { return native(); }

  // Compares numerically, operator== compares raw storage, should work.
  constexpr auto operator<=>(const Int& rhs) const noexcept
    { return (native() <=> rhs.native()); }

  constexpr Int(const SwapT& x) noexcept : Int{T{x}} { }
  constexpr Int& operator=(const SwapT& x) noexcept { return *this = T{x}; }

  constexpr Int operator+()     const noexcept { return *this; }
  constexpr NativeT operator-() const noexcept { return NativeT{-T{*this}}; }
  constexpr Int operator~()     const noexcept { Int x; x._raw = ~_raw; return x; }
  constexpr bool operator!()    const noexcept { return !_raw; }
  constexpr explicit operator bool() const noexcept { return bool(_raw); }

  // Binary ops with T return NativeT.
  constexpr NativeT operator+ (T r) noexcept { return NativeT{T{*this} +  r}; }
  constexpr NativeT operator- (T r) noexcept { return NativeT{T{*this} -  r}; }
  constexpr NativeT operator* (T r) noexcept { return NativeT{T{*this} *  r}; }
  constexpr NativeT operator/ (T r) noexcept { return NativeT{T{*this} /  r}; }
  constexpr NativeT operator% (T r) noexcept { return NativeT{T{*this} %  r}; }
  constexpr NativeT operator<<(T r) noexcept { return NativeT{T{*this} << r}; }
  constexpr NativeT operator>>(T r) noexcept { return NativeT{T{*this} >> r}; }

  constexpr Int& operator+= (T r) noexcept { return *this = *this +  r; }
  constexpr Int& operator-= (T r) noexcept { return *this = *this -  r; }
  constexpr Int& operator*= (T r) noexcept { return *this = *this *  r; }
  constexpr Int& operator/= (T r) noexcept { return *this = *this /  r; }
  constexpr Int& operator%= (T r) noexcept { return *this = *this %  r; }
  constexpr Int& operator<<=(T r) noexcept { return *this = *this << r; }
  constexpr Int& operator>>=(T r) noexcept { return *this = *this >> r; }

  // Forward compound ops with another Int
  constexpr Int& operator+= (Int r) noexcept { return *this +=  T{r}; }
  constexpr Int& operator-= (Int r) noexcept { return *this -=  T{r}; }
  constexpr Int& operator*= (Int r) noexcept { return *this *=  T{r}; }
  constexpr Int& operator/= (Int r) noexcept { return *this /=  T{r}; }
  constexpr Int& operator%= (Int r) noexcept { return *this %=  T{r}; }
  constexpr Int& operator<<=(Int r) noexcept { return *this <<= T{r}; }
  constexpr Int& operator>>=(Int r) noexcept { return *this >>= T{r}; }

  // Bitwise ops never swap.
  constexpr Int& operator|=(Int r) noexcept { _raw |= r._raw; return *this; }
  constexpr Int& operator&=(Int r) noexcept { _raw &= r._raw; return *this; }
  constexpr Int& operator^=(Int r) noexcept { _raw ^= r._raw; return *this; }
  constexpr Int& operator|=(T r)   noexcept { return *this |= Int{r}; }
  constexpr Int& operator&=(T r)   noexcept { return *this &= Int{r}; }
  constexpr Int& operator^=(T r)   noexcept { return *this ^= Int{r}; }
  constexpr Int  operator|(Int r)  noexcept { Int x{*this}; return x |= r; }
  constexpr Int  operator&(Int r)  noexcept { Int x{*this}; return x &= r; }
  constexpr Int  operator^(Int r)  noexcept { Int x{*this}; return x ^= r; }

  constexpr NativeT operator++()     noexcept { T x{*this + T{1}}; *this = x; return NativeT{x}; }
  constexpr NativeT operator--()     noexcept { T x{*this - T{1}}; *this = x; return NativeT{x}; }
  constexpr NativeT operator++(int)  noexcept { T x{*this}; *this = x + T{1}; return NativeT{x}; }
  constexpr NativeT operator--(int)  noexcept { T x{*this}; *this = x - T{1}; return NativeT{x}; }

  // Binary ops, Int rhs
  constexpr NativeT operator+ (Int r) noexcept { return *this +  T{r}; }
  constexpr NativeT operator- (Int r) noexcept { return *this -  T{r}; }
  constexpr NativeT operator* (Int r) noexcept { return *this *  T{r}; }
  constexpr NativeT operator/ (Int r) noexcept { return *this /  T{r}; }
  constexpr NativeT operator% (Int r) noexcept { return *this %  T{r}; }
  constexpr NativeT operator<<(Int r) noexcept { return *this << T{r}; }
  constexpr NativeT operator>>(Int r) noexcept { return *this >> T{r}; }

  // Mixed with T
  friend constexpr NativeT operator+ (T a, Int b) noexcept { return NativeT{a +  T{b}}; }
  friend constexpr NativeT operator- (T a, Int b) noexcept { return NativeT{a -  T{b}}; }
  friend constexpr NativeT operator* (T a, Int b) noexcept { return NativeT{a *  T{b}}; }
  friend constexpr NativeT operator/ (T a, Int b) noexcept { return NativeT{a /  T{b}}; }
  friend constexpr NativeT operator% (T a, Int b) noexcept { return NativeT{a %  T{b}}; }
  friend constexpr NativeT operator| (T a, Int b) noexcept { return NativeT{a |  T{b}}; }
  friend constexpr NativeT operator& (T a, Int b) noexcept { return NativeT{a &  T{b}}; }
  friend constexpr NativeT operator^ (T a, Int b) noexcept { return NativeT{a ^  T{b}}; }
  friend constexpr NativeT operator<<(T a, Int b) noexcept { return NativeT{a << T{b}}; }
  friend constexpr NativeT operator>>(T a, Int b) noexcept { return NativeT{a >> T{b}}; }

#if 0
  static_assert(std::is_standard_layout_v<Int>);
  static_assert(std::is_trivially_copyable_v<Int>);
  static_assert(sizeof(Int) == sizeof(T));
  static_assert(alignof(Int) == alignof(T));
#endif
}; // Int

template<std::integral T, std::endian E>
constexpr std::size_t hash_value(const Int<T, E>& x) noexcept
  { return std::hash<T>{}(x.raw()); }

template<std::endian To, std::integral T, std::endian From>
constexpr Int<T, To> endian_cast(Int<T, From> x) noexcept
  { return Int<T, To>{x}; }

// byteswap must always reverse the physical byte order.
template<std::integral T, std::endian E>
constexpr auto byteswap(Int<T, E> x) noexcept { return Int<T, ~E>{x}; }

template<std::signed_integral   T=int>
using BigInt  = Int<T, std::endian::big>;

template<std::signed_integral   T=int>
using LilInt  = Int<T, std::endian::little>;

template<std::unsigned_integral T=unsigned>
using BigUint = Int<T, std::endian::big>;

template<std::unsigned_integral T=unsigned>
using LilUint = Int<T, std::endian::little>;

} // tjg

namespace std {

template<integral T, endian E>
struct hash<::tjg::Int<T, E>> {
  constexpr size_t operator()(const ::tjg::Int<T, E>& x) noexcept
    { return ::tjg::hash_value(x); } // ::tjg::hash_value() for compatibility with boost::hash.
}; // hash

} // std
