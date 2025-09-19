#pragma once
#include "CrcUpdate.hpp"
#include <functional>
#include <span>
#include <fstream>
#include <bit>
#include <cstdint>

constexpr bool CksumDebug = true;

CrcType CrcSumStream(std::ifstream& stream, std::streamsize* length = nullptr);

CrcType cksum_slice8 (CrcType crc, const void* buf, std::size_t size) noexcept;
CrcType cksum_simd   (CrcType crc, const void* buf, std::size_t size) noexcept;
CrcType cksum_vmull0 (CrcType crc, const void* buf, std::size_t size) noexcept;
CrcType cksum_pclmul0(CrcType crc, const void* buf, std::size_t size) noexcept;

// Buffer is always aligned and a full buffer until size < sizeof(uint128_t)
using GetBufferCb = std::function<std::span<const std::byte>()>;

CrcType cksum_slice8(CrcType crc, GetBufferCb cb) noexcept;
CrcType cksum_simd  (CrcType crc, GetBufferCb cb) noexcept;
