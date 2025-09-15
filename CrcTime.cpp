#include "cksum.hpp"

#include <chrono>
#include <vector>
#include <array>
#include <map>
#include <initializer_list>
#include <span>
#include <random>
#include <concepts>
#include <iostream>
#include <iomanip>
#include <functional>
#include <utility>
#include <type_traits>
#include <cstddef>
#include <cstdlib>

using Clock = std::chrono::steady_clock;

constexpr std::size_t DataSize = (1 << 20);
constexpr int LoopCount = 10;

template<std::unsigned_integral U>
struct CoutType: public std::conditional<(sizeof(U) == 1), unsigned, U> { };

template<std::unsigned_integral U>
auto Value(U x) -> typename CoutType<U>::type
  { return static_cast<typename CoutType<U>::type>(x); }

using CrcType = std::uint32_t;

using CrcFn = CrcType (*)(CrcType crc, const void* buf, std::size_t size);

auto ExpectedCrc = CrcType{0x3e906f85};

std::map<std::string, Clock::duration> Times;;

bool TestCrc(CrcFn fn, const std::string name, std::span<const std::byte> data)
{
  using namespace std;
  using namespace std::chrono;
  // cout << "Running " << left << setw(8) << name << right << "... ";
  auto start = Clock::now();
  auto crc = CrcType{};
  for (int i = 0; i != LoopCount; ++i)
    crc = fn(crc, data.data(), data.size());
  auto stop = Clock::now();
  Times[name] += (stop - start);
#if 0
  auto s = duration<double>(stop - start);
  auto rate = static_cast<double>(data.size() * LoopCount)
            / DataSize / s.count();
  cout << hex << setfill('0') << " 0x" << setw(8) << crc
    << dec << setfill(' ') << fixed << setprecision(0)
    << ' ' << setw(8) << rate << "MiB/s";
  if (ExpectedCrc == CrcType{})
    ExpectedCrc = crc;
  if (crc != ExpectedCrc)
    cout << " FAIL";
  cout << endl;
#endif
  return (crc == ExpectedCrc);
} // TestCrc

int main() {
  constexpr auto Seed = 12345;
  std::mt19937 rng{Seed};

  std::cerr << "Generating " << DataSize << " random bytes " << std::flush;
  // Fill data with pseudo-random bytes
  std::vector<std::byte> data;
  {
    rng.seed(Seed);
    data.reserve(DataSize);
    for (int i = 0; i != DataSize; ++i)
      data.push_back(std::byte(rng() & 0xff));
  }
  std::cerr << "done." << std::endl;

  int failed = 0;

  constexpr int loops = 30;
  for (int i = 0; i != loops; ++i) {
    failed += !TestCrc(cksum_slice8 , "Slice8" , std::span{data});
#ifdef USE_VMULL_CRC32
    failed += !TestCrc(cksum_simd   , "Simd"   , std::span{data});
    failed += !TestCrc(cksum_vmull0 , "Vmull0" , std::span{data});
#endif
#ifdef USE_PCLMUL_CRC32
    failed += !TestCrc(cksum_simd   , "Simd"   , std::span{data});
    failed += !TestCrc(cksum_pclmul0, "PclMul0", std::span{data});
#endif
  }

  for (const auto& [name, dt]: Times) {
    using namespace std;
    auto s = chrono::duration<double>(dt);
    auto rate = static_cast<double>(data.size() * LoopCount * loops)
              / DataSize / s.count();
    cout << left << setw(8) << name << right
         << fixed << setprecision(0) << ' ' << setw(8) << rate << " MiB/s\n";
  }

  if (failed != 0) {
    std::cout << "\nFailed " << failed << " tests.\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
} // main
