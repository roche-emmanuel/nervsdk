#ifndef _NVK_TYPES_
#define _NVK_TYPES_

#ifdef _WIN32
#include <windows.h>
#include <windowsx.h>
#endif

#include <any>
#include <cmath>
#include <cstdint>
#include <limits.h>

namespace nv {

using VoidPtr = void*;
using Bool = bool;
using Byte = unsigned char;
using I8 = std::int8_t;
using U8 = std::uint8_t;
using I16 = std::int16_t;
using U16 = std::uint16_t;
using I32 = std::int32_t;
using U32 = std::uint32_t;
using I64 = std::int64_t;
using U64 = std::uint64_t;
using StringID = U64;
using Float = std::float_t;
using F32 = std::float_t;
using Double = std::double_t;
using F64 = std::double_t;

using ResourceType = StringID;
using ComponentType = StringID;

constexpr U64 U64_MAX = ULLONG_MAX;
constexpr U32 U32_MAX = UINT_MAX;

} // namespace nv

#ifndef _WIN32
// Manually define DWORD type here:
using DWORD = nv::U32;
#endif

#endif
