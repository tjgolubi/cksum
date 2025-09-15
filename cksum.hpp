#pragma once
#include <fstream>
#include <cstdint>

constexpr bool CksumDebug = true;

std::uint32_t CrcSumStream(std::ifstream& stream, std::streamsize* length = nullptr);

std::uint32_t cksum_slice8(std::uint32_t crc, const void* buf, std::size_t size) noexcept;
std::uint32_t cksum_slice8a(std::uint32_t crc, const void* buf, std::size_t size) noexcept;
std::uint32_t cksum_slice8b(std::uint32_t crc, const void* buf, std::size_t size) noexcept;
std::uint32_t cksum_simd(std::uint32_t crc, const void* buf, std::size_t size) noexcept;
std::uint32_t cksum_vmull0(std::uint32_t crc, const void* buf, std::size_t size) noexcept;
std::uint32_t cksum_pclmul0(std::uint32_t crc, const void* buf, std::size_t size) noexcept;
