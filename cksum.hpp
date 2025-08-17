/* Calculate checksums and sizes of files
   Copyright 2025 Free Software Foundation, Inc. */

#pragma once

#include <fstream>
#include <cstdint>

using uint128_t = unsigned __int128;

constexpr bool CksumDebug = true;

std::uint32_t CrcSumStream(std::ifstream& stream, std::streamsize* length = nullptr);

extern const std::uint_fast32_t CrcTab[8][256];

std::uint_fast32_t cksum_slice8(std::uint_fast32_t crc, void* buf, std::size_t* bufsize) noexcept;
std::uint_fast32_t cksum_vmull (std::uint_fast32_t crc, void* buf, std::size_t* bufsize) noexcept;
std::uint_fast32_t cksum_pclmul(std::uint_fast32_t crc, void* buf, std::size_t* bufsize) noexcept;
