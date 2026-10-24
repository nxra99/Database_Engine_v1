#ifndef TYPDEFS_H
#define TYPDEFS_H

#pragma once

#include <iostream>
#include <cstdint>
#include <vector>
#include <memory>
#include <cstring>
#include <cassert>
#include <array>
#include <fcntl.h>      // open(), O_RDWR, O_CREAT, etc.
#include <unistd.h>     // read(), write(), close(), lseek(), fdatasync(), ftruncate()
#include <sys/stat.h>  
#include <variant>
#include <unordered_map>
#include <optional>
#include <string>
#include <string_view> 

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef uint8_t FRAME_IDX;

constexpr u32 PAGE_SIZE = 4096;
constexpr u16 HEADER_SIZE = 24;
constexpr u16 SLOT_SIZE = 10;





















//(de)serialisation
static void write_u8_le(u8* dst, u8 value) 
{
	dst[0] = value;
}

static u8 read_u8_le(const u8* src) 
{
	return src[0];
}

static void write_u16_le(u8* dst, u16 value)
{
  dst[0] = static_cast<u8>(value & 0xFF); 
  dst[1] = static_cast<u8>((value >> 8) & 0xFF);
}

static u16 read_u16_le(const u8* src)
{
  return static_cast<u16>(src[0]) | (static_cast<u16>(src[1]) << 8);
}

static void write_u32_le(u8* dst, u32 value)
{
  dst[0] = static_cast<u8>(value & 0xFF);
  dst[1] = static_cast<u8>((value >> 8) & 0xFF);
  dst[2] = static_cast<u8>((value >> 16) & 0xFF);
  dst[3] = static_cast<u8>((value >> 24) & 0xFF);
}

static u32 read_u32_le(const u8* src)
{
  return static_cast<u32>(src[0]) | (static_cast<u32>(src[1]) << 8) | (static_cast<u32>(src[2]) << 16) | (static_cast<u32>(src[3]) << 24); 
}

static void write_u64_le(u8* dst, u64 value)
{
  dst[0] = static_cast<u8>(value & 0xFF);
  dst[1] = static_cast<u8>((value >> 8) & 0xFF);
  dst[2] = static_cast<u8>((value >> 16) & 0xFF);
  dst[3] = static_cast<u8>((value >> 24) & 0xFF); 
  dst[4] = static_cast<u8>((value >> 32) & 0xFF);
  dst[5] = static_cast<u8>((value >> 40) & 0xFF);
  dst[6] = static_cast<u8>((value >> 48) & 0xFF);
  dst[7] = static_cast<u8>((value >> 56) & 0xFF);
}

static u64 read_u64_le(const u8* src)
{
  return static_cast<u64>(src[0]) | (static_cast<u64>(src[1]) << 8) | (static_cast<u64>(src[2]) << 16) | (static_cast<u64>(src[3]) << 24) | 
  (static_cast<u64>(src[4]) << 32) | (static_cast<u64>(src[5]) << 40) | (static_cast<u64>(src[6]) << 48) | (static_cast<u64>(src[7]) << 56);
}





#endif