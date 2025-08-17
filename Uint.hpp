#pragma once

#include <concepts>
#include <bit>

template<std::unsigned_integral T, std::endian Rep = std::endian::native>
class Uint {
public:
  using value_type = T;
  static constexpr auto endian_type = Rep;

  static constexpr std::endian Flip(std::endian x) noexcept {
    if (x == std::endian::little)
      return std::endian::big;
    else
      return std::endian::little;
  }

private:
  T _storage = T{0};

  // T is in native endian.
  constexpr void store(T x) noexcept {
    if constexpr (Rep == std::endian::native)
      _storage = x;
    else
      _storage = std::byteswap(x);
  }

public:
  constexpr Uint() noexcept = default;

  constexpr explicit Uint(T x) noexcept { store(x); }

  constexpr T raw() const noexcept { return _storage; }

  template<std::endian As>
  constexpr T as() const noexcept {
    if constexpr (As == Rep)
      return _storage;
    else
      return std::byteswap(_storage);
  }

  constexpr T value()  const noexcept { return as<std::endian::native>(); }
  constexpr T little() const noexcept { return as<std::endian::little>(); }
  constexpr T big()    const noexcept { return as<std::endian::big>();    }

  constexpr operator T() const noexcept { return value(); }

  constexpr Uint& operator=(const Uint&) noexcept = default;

  constexpr Uint& operator=(T x) noexcept { store(x); return *this; }

  constexpr auto operator<=>(const Uint& rhs) const { return (value() <=> rhs.value()); }

  template<std::endian Rep2> requires (Rep2 != Rep)
  constexpr Uint(Uint<T, Rep2> x) noexcept { store(x.value()); }

  template<std::endian Rep2> requires (Rep2 != Rep)
  constexpr Uint& operator=(const Uint<T, Rep2>& x) noexcept {
    store(x.value());
    return *this;
  }

  constexpr auto byteswap() const noexcept
    { return static_cast<Uint<T, Flip(Rep)>>(*this); }

#if 0
  constexpr auto reflect() const noexcept {
    auto x = *this;
    x._storage = tjg::Reflect(x._storage);
    return x;
  }
#endif
}; // Uint

namespace std {

template<std::unsigned_integral T, std::endian Rep>
auto byteswap(Uint<T, Rep> x) noexcept {
  return x.byteswap();
}

} // std
