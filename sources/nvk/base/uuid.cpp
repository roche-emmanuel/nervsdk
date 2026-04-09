// file: sources/nvk/base/uuid.cpp

#include <nvk/base/RandGen.h>
#include <nvk/base/uuid.h>


#include <random>
#include <stdexcept>

namespace nv {

namespace {

/** Random source for UUID generation.

    We deliberately do NOT use the engine-wide RandGen::instance() because
    that singleton is seeded with a fixed value (1234) for reproducibility
    of procedural content. UUIDs need real entropy, so we keep a separate
    thread-local RandGen seeded from std::random_device. Thread-local
    avoids any contention and means each thread is independently seeded.

    A version-4 UUID needs 122 bits of randomness; MT19937 with a real
    random_device seed easily satisfies our uniqueness requirements (the
    expected collision count for ~1 billion UUIDs is below 1e-9). */
auto get_uuid_rand_gen() -> std::mt19937& {
    thread_local std::mt19937 gen(std::random_device{}());
    return gen;
}

constexpr char HEX_DIGITS[] = "0123456789abcdef";

auto hex_value(char c) -> I32 {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return 10 + (c - 'a');
    }
    if (c >= 'A' && c <= 'F') {
        return 10 + (c - 'A');
    }
    return -1;
}

auto parse_byte(const char* p) -> I32 {
    I32 hi = hex_value(p[0]);
    I32 lo = hex_value(p[1]);
    if (hi < 0 || lo < 0) {
        return -1;
    }
    return (hi << 4) | lo;
}

} // namespace

Uuid::Uuid() = default;

Uuid::Uuid(const U8 (&bytes)[BYTE_COUNT]) {
    for (U32 i = 0; i < BYTE_COUNT; ++i) {
        _bytes[i] = bytes[i];
    }
}

auto Uuid::is_nil() const -> bool {
    for (U32 i = 0; i < BYTE_COUNT; ++i) {
        if (_bytes[i] != 0) {
            return false;
        }
    }
    return true;
}

auto Uuid::generate_v4() -> Uuid {
    Uuid result;
    auto& gen = get_uuid_rand_gen();

    // Pull 16 random bytes. We pull 32-bit words four at a time and split
    // them rather than calling the generator 16 times.
    std::uniform_int_distribution<U32> dist(0, 0xFFFFFFFFu);
    for (U32 i = 0; i < 4; ++i) {
        U32 word = dist(gen);
        result._bytes[i * 4 + 0] = static_cast<U8>(word & 0xFF);
        result._bytes[i * 4 + 1] = static_cast<U8>((word >> 8) & 0xFF);
        result._bytes[i * 4 + 2] = static_cast<U8>((word >> 16) & 0xFF);
        result._bytes[i * 4 + 3] = static_cast<U8>((word >> 24) & 0xFF);
    }

    // Set the version (4) in the upper nibble of byte 6.
    result._bytes[6] = static_cast<U8>((result._bytes[6] & 0x0F) | 0x40);

    // Set the variant (RFC 4122, "10xx") in the upper bits of byte 8.
    result._bytes[8] = static_cast<U8>((result._bytes[8] & 0x3F) | 0x80);

    return result;
}

auto Uuid::generate_v4_string() -> String { return generate_v4().to_string(); }

auto Uuid::to_string() const -> String {
    // Format: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
    // Byte layout: 4-2-2-2-6 bytes, separated by hyphens.
    String result(STRING_LENGTH, '-');
    static constexpr U32 GROUP_BYTES[] = {0, 4, 6, 8, 10};
    static constexpr U32 GROUP_LEN[] = {4, 2, 2, 2, 6};
    static constexpr U32 GROUP_OUT[] = {0, 9, 14, 19, 24};

    for (U32 g = 0; g < 5; ++g) {
        U32 in = GROUP_BYTES[g];
        U32 out = GROUP_OUT[g];
        for (U32 i = 0; i < GROUP_LEN[g]; ++i) {
            U8 b = _bytes[in + i];
            result[out + i * 2 + 0] = HEX_DIGITS[(b >> 4) & 0x0F];
            result[out + i * 2 + 1] = HEX_DIGITS[b & 0x0F];
        }
    }
    return result;
}

auto Uuid::try_from_string(const String& str, Uuid& out) -> bool {
    if (str.size() != STRING_LENGTH) {
        return false;
    }
    // Verify hyphen positions.
    if (str[8] != '-' || str[13] != '-' || str[18] != '-' || str[23] != '-') {
        return false;
    }

    static constexpr U32 GROUP_BYTES[] = {0, 4, 6, 8, 10};
    static constexpr U32 GROUP_LEN[] = {4, 2, 2, 2, 6};
    static constexpr U32 GROUP_IN[] = {0, 9, 14, 19, 24};

    Uuid tmp;
    for (U32 g = 0; g < 5; ++g) {
        U32 out_byte = GROUP_BYTES[g];
        U32 in_char = GROUP_IN[g];
        for (U32 i = 0; i < GROUP_LEN[g]; ++i) {
            I32 byte = parse_byte(str.data() + in_char + i * 2);
            if (byte < 0) {
                return false;
            }
            tmp._bytes[out_byte + i] = static_cast<U8>(byte);
        }
    }
    out = tmp;
    return true;
}

auto Uuid::from_string(const String& str) -> Uuid {
    Uuid result;
    if (!try_from_string(str, result)) {
        throw std::invalid_argument("Invalid UUID string: " + str);
    }
    return result;
}

} // namespace nv