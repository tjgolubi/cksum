/// @file
/// @copyright 2025 Terry Golubiewski, all rights reserved.
/// @author Terry Golubiewski
/// @brief Strongly typed integer wrapper with explicit byte-order semantics.
/// Provides safe, non-narrowing construction and assignment.
/// @details
/// Defines ::tjg::Int<T, E> which stores an integral value using a specified
/// std::endian. It offers constexpr accessors for host- and storage-order views,
/// constrained conversions via NonNarrowing, ordering, hashing, and explicit
/// helpers like endian_cast and byteswap.

#pragma once
#include <concepts>   // std::integral
#include <functional> // std::hash
#include <type_traits>// std::is_trivially_copyable_t
#include <compare>    // operator<=>
#include <bit>        // std::endian, std::byteswap
#include <memory>     // std::addressof
#include <utility>    // std::declval
#include <cstddef>    // std::size_t

namespace std {

/// Tilde operator returns the opposite endianness.
constexpr endian operator~(endian x) noexcept {
  if (x == endian::little)
    return endian::big;
  else
    return endian::little;
}

} // std

namespace tjg {

/// Convert between integral types without narrowing. Use when an implicit
/// conversion could lose range.
/// @param x value to convert
template<typename To, typename From>
constexpr To narrow_cast(From x) noexcept { return static_cast<To>(x); }

/// Concept that accepts only non-narrowing conversions between integral types.
template<class From, class To>
concept NonNarrowing = std::same_as<From, To> || (std::convertible_to<From, To>
              && requires (From x) { To{ x }; }); // list-init rejects narrowing

/// Fixed-endian integer that stores its value using byte order E while exposing
/// normal integer semantics.
template<std::integral T=int, std::endian E = std::endian::native>
requires (std::same_as<T, std::remove_cv_t<T>> && !std::is_reference_v<T>)
class Int {
public:
  using value_type = T;
  static constexpr std::endian Endian = E;

private:
  value_type _raw = value_type{0};

  template<std::endian Rep>
  constexpr T _get() const noexcept {
    if constexpr (Endian == Rep)
      return _raw;
    else
      return std::byteswap(_raw);
  }

  constexpr void _set(T x) noexcept {
    if constexpr (Endian == std::endian::native)
      _raw = x;
    else
      _raw = std::byteswap(x);
  }

public:
  /// @name Constructors
  /// @{
  constexpr Int() noexcept = default;
  constexpr Int(const Int&) noexcept = default;

  /// Construct from the underlying type. The value is stored using endianness E.
  /// construction is explicit to avoid surprises in mixed expressions.
  constexpr explicit Int(T x) noexcept { _set(x); }
  /// @}

  /// @name Assignment
  /// @{
  constexpr Int& operator=(const Int&) = default;

  /// Allow only non-narrowing assignment.
  template<std::integral U> requires NonNarrowing<U, T>
  constexpr Int& operator=(U x) noexcept { _set(T{x}); return *this; }

  /// Assign from another Int<U, E2> without narrowing.
  template<std::integral U, std::endian E2> requires NonNarrowing<U, T>
  constexpr Int& operator=(Int<U, E2> x) noexcept
    { _set(T{U{x}}); return *this; }
  /// @}

private:
  // Specifically delete narrowing assignment.
  template<std::integral U> requires (!NonNarrowing<U, T>)
  constexpr Int& operator=(U x) noexcept = delete;

  template<std::integral U, std::endian E2> requires (!NonNarrowing<U, T>)
  constexpr Int& operator=(Int<U, E2> x) noexcept = delete;

public:
  /// @name Accessors.
  /// @{
  /// Return a reference to the underlying stored value (in storage endianness).
  /// @{
  constexpr       T& raw()       noexcept { return _raw;   }
  constexpr const T& raw() const noexcept { return _raw;   }
  /// @}

  /// Return a pointer to the underlying stored value.
  /// @{
  constexpr       T* ptr()       noexcept
    requires (Endian == std::endian::native)
    { return std::addressof(_raw); }

  constexpr const T* ptr() const noexcept
    requires (Endian == std::endian::native)
    { return std::addressof(_raw); }
  /// @}

  /// Return the numeric value in host byte order.
  [[nodiscard]] constexpr T value() const noexcept
    { return _get<std::endian::native>(); }

  /// Return the value in big-endian byte order.
  [[nodiscard]] constexpr T big() const noexcept
    { return _get<std::endian::big>(); }

  /// Return the value in little-endian byte order.
  [[nodiscard]] constexpr T little() const noexcept
    { return _get<std::endian::little>(); }

  constexpr operator T() const noexcept { return value(); }
  /// @}

  /// @name Comparison
  /// @{
  constexpr bool operator==(const Int&) const = default;

  // Compares numerically, operator== compares raw storage, should work.
  constexpr auto operator<=>(const Int& rhs) const noexcept
    { return (value() <=> rhs.value()); }
  /// @}

/// Unary operators mirror the underlying integer semantics and return an Int
/// when no byteswap is necessary.
/// @name Unary operators
/// @{
  constexpr Int operator+() const noexcept { return *this; }
  constexpr auto operator-() const noexcept { return -value(); }
  constexpr Int operator~()  const noexcept
    { Int x; x._raw = narrow_cast<T>(~_raw); return x; }
  constexpr bool operator!() const noexcept { return !_raw; }
  constexpr explicit operator bool() const noexcept { return bool(_raw); }
/// @}

/// Pre- and post-increment/decrement operate on the numeric value and follow
/// standard integer rules.
/// @name Increment and decrement
/// @{
  constexpr Int& operator++() noexcept
    { return *this = static_cast<T>(value() + 1); }

  constexpr Int& operator--() noexcept
    { return *this = static_cast<T>(value() - 1); }

  constexpr T operator++(int) noexcept {
    T prev = value();
    *this = static_cast<T>(prev + 1);
    return prev;
  }

  constexpr T operator--(int) noexcept {
    T prev = value();
    *this = static_cast<T>(prev - 1);
    return prev;
  }
/// @}

/// Compound assignment operators update the numeric value in place; scalar
/// right-hand sides are constrained to non-narrowing conversions.
/// @name Compound assignment
/// @{
  template<std::integral U> requires NonNarrowing<U, T>
  constexpr Int& operator+=(U rhs) noexcept
    { return *this = narrow_cast<T>(value() + rhs); }

  template<std::integral U> requires NonNarrowing<U, T>
  constexpr Int& operator-=(U rhs) noexcept
    { return *this = narrow_cast<T>(value() - rhs); }

  template<std::integral U> requires NonNarrowing<U, T>
  constexpr Int& operator*=(U rhs) noexcept
    { return *this = narrow_cast<T>(value() * rhs); }

  template<std::integral U> requires NonNarrowing<U, T>
  constexpr Int& operator/=(U rhs) noexcept
    { return *this = narrow_cast<T>(value() / rhs); }

  template<std::integral U> requires NonNarrowing<U, T>
  constexpr Int& operator%=(U rhs) noexcept
    { return *this = narrow_cast<T>(value() % rhs); }

  constexpr Int& operator<<=(std::integral auto rhs) noexcept
    { return *this = static_cast<T>(value() << rhs); }

  constexpr Int& operator>>=(std::integral auto rhs) noexcept
    { return *this = static_cast<T>(value() >> rhs); }

  constexpr Int& operator +=(Int rhs) noexcept { return *this  += T{rhs}; }
  constexpr Int& operator -=(Int rhs) noexcept { return *this  -= T{rhs}; }
  constexpr Int& operator *=(Int rhs) noexcept { return *this  *= T{rhs}; }
  constexpr Int& operator /=(Int rhs) noexcept { return *this  /= T{rhs}; }
  constexpr Int& operator %=(Int rhs) noexcept { return *this  %= T{rhs}; }
  constexpr Int& operator<<=(Int rhs) noexcept { return *this <<= T{rhs}; }
  constexpr Int& operator>>=(Int rhs) noexcept { return *this >>= T{rhs}; }

  constexpr Int& operator|=(Int rhs) noexcept
    { _raw |= rhs._raw; return *this; }
  constexpr Int& operator&=(Int rhs) noexcept
    { _raw &= rhs._raw; return *this; }
  constexpr Int& operator^=(Int rhs) noexcept
    { _raw ^= rhs._raw; return *this; }

  template<std::integral U> requires NonNarrowing<U, T>
  constexpr Int& operator|=(U rhs) noexcept { return *this |= Int{T{rhs}}; }

  template<std::integral U> requires NonNarrowing<U, T>
  constexpr Int& operator&=(U rhs) noexcept { return *this &= Int{T{rhs}}; }

  template<std::integral U> requires NonNarrowing<U, T>
  constexpr Int& operator^=(U rhs) noexcept { return *this ^= Int{T{rhs}}; }
  /// @}

  /// Bitwise operators act on the numeric value and return Int.
  /// These and, or, and xor operations don't require byteswaps.
  /// @name Bitwise operators
  /// @{
  constexpr Int operator|(Int rhs) const noexcept { return Int{*this} |= rhs; }
  constexpr Int operator&(Int rhs) const noexcept { return Int{*this} &= rhs; }
  constexpr Int operator^(Int rhs) const noexcept { return Int{*this} ^= rhs; }
  /// @}
}; // Int

/// Deduce integral type from constructor argument.
template<std::integral U>
Int(U) -> Int<U, std::endian::native>;

/// Deduce from another Int keeps its T and E (copy/cross-endian cases)
template<std::integral U, std::endian E>
Int(Int<U, E>) -> Int<U, E>;

/// Verify instantiation of Int<T, E> is standard-layout, without padding.
/// For use in test code, if you can instantiate a VerifyInt<T>, then Int<T> is
/// safe to use in spans and packed messages.
template<std::integral T>
requires (std::is_standard_layout_v<Int<T>>
      && std::is_trivially_copyable_v<Int<T>>
      && sizeof (Int<T>) == sizeof(T)
      && alignof(Int<T>) == alignof(T))
struct VerifyInt { };

/// Compute a hash of Int<T, E> based on its host-order numeric value; suitable
/// for unordered containers.
/// For boost::hash compatibility.
/// @param x integer wrapper
/// @return hash of the value in host order
template<std::integral T, std::endian E>
constexpr std::size_t hash_value(const Int<T, E>& x) noexcept
  { return static_cast<std::size_t>(x.raw()); }

/// Construct an Int<T, To> from a value expressed in byte order From,
/// converting as needed.
///
/// @tparam T underlying integral type
/// @tparam To destination endianness
/// @tparam From source endianness
/// @param x input value in byte order From
/// @return Int<T, To> storing x in To
template<std::endian To, std::integral T, std::endian From>
constexpr Int<T, To> endian_cast(Int<T, From> x) noexcept
  { return Int<T, To>{x}; }

/// Typecast for underlying integral of an Int.
/// Usually used to assign to a narrower Int.
/// If E is non-native endian, then one byteswap will result.
/// The result is always in native endian.
/// @tparam ToT The desired underlying type for the result.
/// @tparam FmT The (usually deduced) underlying type of the argument.
/// @tparam E   The (usually deduced) endianness of the argument.
/// @param x Int type to typecast.
/// @return Int with specified underlying type (always native endian).
template<std::integral ToT, std::integral FmT, std::endian E>
requires (!std::same_as<ToT, FmT>)
constexpr Int<ToT, std::endian::native> narrow_cast(Int<FmT, E> x) noexcept
  { return Int<ToT, std::endian::native>{narrow_cast<ToT>(x.value())}; }

/// Typecast for underlying integral of an Int.
/// If the type is the same, do nothing.
template<std::integral ToT, std::integral FmT, std::endian E>
requires std::same_as<ToT, FmT>
constexpr Int<ToT, E> narrow_cast(Int<FmT, E> x) noexcept
  { return x; }

/// Return a value with the opposite endianness by byte-swapping the underlying
/// bits.
/// @tparam T underlying integral type
/// @tparam E current endianness
/// @param x value to flip
/// @return Int<T, ~E>
template<std::integral T, std::endian E>
constexpr Int<T, ~E> byteswap(Int<T, E> x) noexcept
  { return Int<T, ~E>{x}; }

/// Alias for big-endian signed Int with underlying type T.
template<std::signed_integral   T=int>
using BigInt    = Int<T, std::endian::big>;
using BigInt8   = BigInt<std::int8_t>;
using BigInt16  = BigInt<std::int16_t>;
using BigInt32  = BigInt<std::int32_t>;
using BigInt64  = BigInt<std::int64_t>;

/// Alias for little-endian signed Int with underlying type T.
template<std::signed_integral   T=int>
using LilInt    = Int<T, std::endian::little>;
using LilInt8   = LilInt<std::int8_t>;
using LilInt16  = LilInt<std::int16_t>;
using LilInt32  = LilInt<std::int32_t>;
using LilInt64  = LilInt<std::int64_t>;

/// Alias for big-endian unsigned Int with underlying type T.
template<std::unsigned_integral T=unsigned>
using BigUint    = Int<T, std::endian::big>;
using BigUint8   = BigUint<std::uint8_t>;
using BigUint16  = BigUint<std::uint16_t>;
using BigUint32  = BigUint<std::uint32_t>;
using BigUint64  = BigUint<std::uint64_t>;


/// Alias for little-endian unsigned Int with underlying type T.
template<std::unsigned_integral T=unsigned>
using LilUint    = Int<T, std::endian::little>;
using LilUint8   = LilUint<std::uint8_t>;
using LilUint16  = LilUint<std::uint16_t>;
using LilUint32  = LilUint<std::uint32_t>;
using LilUint64  = LilUint<std::uint64_t>;

} // tjg

namespace std {

/// std::hash specialization forwarding to ::tjg::hash_value(Int);
/// enables use of Int in unordered containers.
template<integral T, endian E>
struct hash<::tjg::Int<T, E>> {
  constexpr size_t operator()(const ::tjg::Int<T, E>& x) const noexcept
    { return ::tjg::hash_value(x); }
}; // hash

} // std
