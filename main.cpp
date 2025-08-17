#include "cksum.hpp"

#include <string_view>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstdint>
#include <cstdlib>

int main(int argc, const char* argv[]) {
  using namespace std::literals;
  namespace fs = std::filesystem;
  if (argc < 2) {
    std::cerr << "usage: cksum file...\n";
    return EXIT_FAILURE;
  }
  if (std::string_view{argv[1]} == "--version"sv) {
    std::cout << "cksum (coreutils-9.7)\n";
    return EXIT_SUCCESS;
  }
  for (int i = 1; i != argc; ++i) {
    auto fname = fs::path{argv[i]};
    auto fp = std::ifstream{fname, std::ios::in | std::ios::binary};
    if (!fp) {
      std::cerr << fname.generic_string() << ": cannot read\n";
      continue;
    }
    auto file_length = std::streamsize{0};
    auto crc_result = CrcSumStream(fp, &file_length);
    std::cout << crc_result << ' ' << file_length
              << ' ' << fname.generic_string() << '\n';
    fp.close();
  }
  return EXIT_SUCCESS;
} // main
