#ifndef _STRING_ID_H_
#define _STRING_ID_H_

#include <nvk_types.h>

namespace nv {

// FNV-1a hashing support:

// fnv1a 32 and 64 bit hash functions
//  key is the data to hash, len is the size of the data (or how much of it to
//  hash against) code license: public domain or equivalent post:
//  https://notes.underscorediscovery.com/constexpr-fnv1a/

inline auto hash_32_fnv1a(const void* key, const U32 len) -> U32 {

    const char* data = (char*)key;
    U32 hash = 0x811c9dc5;
    U32 prime = 0x1000193;

    for (U32 i = 0; i < len; ++i) {
        U8 value = data[i];
        hash = hash ^ value;
        hash *= prime;
    }

    return hash;

} // hash_32_fnv1a

inline auto hash_64_fnv1a(const void* key, const U64 len) -> U64 {

    const char* data = (char*)key;
    U64 hash = 0xcbf29ce484222325;
    U64 prime = 0x100000001b3;

    for (U64 i = 0; i < len; ++i) {
        U8 value = data[i];
        hash = hash ^ value;
        hash *= prime;
    }

    return hash;

} // hash_64_fnv1a

// FNV1a c++11 constexpr compile time hash functions, 32 and 64 bit
// str should be a null terminated string literal, value should be left out
// e.g hash_32_fnv1a_const("example")
// code license: public domain or equivalent
// post: https://notes.underscorediscovery.com/constexpr-fnv1a/

constexpr U32 val_32_const = 0x811c9dc5;
constexpr U32 prime_32_const = 0x1000193;
constexpr U64 val_64_const = 0xcbf29ce484222325;
constexpr U64 prime_64_const = 0x100000001b3;

inline constexpr auto
hash_32_fnv1a_const(const char* const str,
                    const U32 value = val_32_const) noexcept -> U32 {
    return (str[0] == '\0')
               ? value
               : hash_32_fnv1a_const(&str[1],
                                     (value ^ U32(str[0])) * prime_32_const);
}

inline constexpr auto
hash_64_fnv1a_const(const char* const str,
                    const U64 value = val_64_const) noexcept -> U64 {
    return (str[0] == '\0')
               ? value
               : hash_64_fnv1a_const(&str[1],
                                     (value ^ U64(str[0])) * prime_64_const);
}

// Implementation of StringID mechanism:

// cf. https://en.cppreference.com/w/cpp/language/user_literal
inline constexpr auto operator""_sid(const char* str,
                                     std::size_t /*n*/) noexcept -> StringID {
    return hash_64_fnv1a_const(str);
}

inline auto str_id(const char* str, std::size_t n) noexcept -> StringID {
    return hash_64_fnv1a(str, n);
}

inline auto str_id(const char* str) noexcept -> StringID {
    return hash_64_fnv1a(str, strlen(str));
}

inline auto str_id(const String& str) noexcept -> StringID {
    return hash_64_fnv1a(str.data(), str.size());
}

inline constexpr auto str_id_const(const char* str) noexcept -> StringID {
    return hash_64_fnv1a_const(str);
}

} // namespace nv

// String ID generation macro: //NOLINTNEXTLINE
#define SID(str) nv::str_id_const(str)

#endif
