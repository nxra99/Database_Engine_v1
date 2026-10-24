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
typedef uint8_t FIDX;

constexpr u32 PAGE_SIZE = 4096;
constexpr u16 HEADER_SIZE = 24;
constexpr u16 SLOT_SIZE = 10;





















//(de)serialisation





#endif