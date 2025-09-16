#pragma once
#include "CrcUpdate.hpp"
#include <fstream>
#include <cstdint>

constexpr bool CksumDebug = true;

CrcType CrcSumStream(std::ifstream& stream, std::streamsize* length = nullptr);

CrcType cksum_slice8 (CrcType crc, const void* buf, std::size_t size) noexcept;
CrcType cksum_simd   (CrcType crc, const void* buf, std::size_t size) noexcept;
CrcType cksum_vmull0 (CrcType crc, const void* buf, std::size_t size) noexcept;
CrcType cksum_pclmul0(CrcType crc, const void* buf, std::size_t size) noexcept;
