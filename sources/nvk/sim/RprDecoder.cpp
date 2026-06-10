#include <nvk/sim/RprDecoder.h>

#include <cassert>
#include <cstring> // memcpy

namespace nv {

// =============================================================================
// Big-endian primitive readers
//
// We use memcpy to copy the raw bytes into a temporary integer before
// reinterpreting as float/double. This is the correct portable approach:
// - It avoids strict-aliasing UB (no pointer casts between unrelated types).
// - Modern compilers optimise it to a single bswap + load instruction.
// =============================================================================

auto RprDecoder::read_u16_be(const U8* p) noexcept -> U16 {
    return static_cast<U16>((static_cast<U16>(p[0]) << 8) |
                            static_cast<U16>(p[1]));
}

auto RprDecoder::read_f32_be(const U8* p) noexcept -> F32 {
    U32 raw = (static_cast<U32>(p[0]) << 24) | (static_cast<U32>(p[1]) << 16) |
              (static_cast<U32>(p[2]) << 8) | static_cast<U32>(p[3]);
    F32 result;
    memcpy(&result, &raw, sizeof(result));
    return result;
}

auto RprDecoder::read_f64_be(const U8* p) noexcept -> F64 {
    U64 raw = (static_cast<U64>(p[0]) << 56) | (static_cast<U64>(p[1]) << 48) |
              (static_cast<U64>(p[2]) << 40) | (static_cast<U64>(p[3]) << 32) |
              (static_cast<U64>(p[4]) << 24) | (static_cast<U64>(p[5]) << 16) |
              (static_cast<U64>(p[6]) << 8) | static_cast<U64>(p[7]);
    F64 result;
    memcpy(&result, &raw, sizeof(result));
    return result;
}

// =============================================================================
// Attribute decoders
// =============================================================================

auto RprDecoder::decode_world_location(const U8* buf, size_t len) -> Vec3d {
    assert(buf != nullptr);
    assert(len >= 24 &&
           "WorldLocationStruct requires 24 bytes (3 x HLAfloat64BE)");
    return Vec3d(read_f64_be(buf + 0), // X
                 read_f64_be(buf + 8), // Y
                 read_f64_be(buf + 16) // Z
    );
}

auto RprDecoder::decode_orientation(const U8* buf, size_t len) -> Vec3f {
    assert(buf != nullptr);
    assert(len >= 12 &&
           "OrientationStruct requires 12 bytes (3 x HLAfloat32BE)");
    return Vec3f(read_f32_be(buf + 0), // Psi   (yaw)
                 read_f32_be(buf + 4), // Theta (pitch)
                 read_f32_be(buf + 8)  // Phi   (roll)
    );
}

auto RprDecoder::decode_entity_type(const U8* buf, size_t len)
    -> RprEntityType {
    assert(buf != nullptr);
    assert(len >= 8 && "EntityTypeStruct requires 8 bytes");
    RprEntityType t;
    t.kind = buf[0];
    t.domain = buf[1];
    t.countryCode = read_u16_be(buf + 2);
    t.category = buf[4];
    t.subcategory = buf[5];
    t.specific = buf[6];
    t.extra = buf[7];
    return t;
}

auto RprDecoder::decode_entity_id(const U8* buf, size_t len) -> U64 {
    assert(buf != nullptr);
    assert(len >= 6 &&
           "EntityIdentifierStruct requires 6 bytes (3 x HLAinteger16BE)");
    const U64 site = read_u16_be(buf + 0);
    const U64 app = read_u16_be(buf + 2);
    const U64 entNr = read_u16_be(buf + 4);
    // Pack into a single U64: [unused 16 bits][site 16][app 16][entity 16]
    return (site << 32) | (app << 16) | entNr;
}

} // namespace nv
