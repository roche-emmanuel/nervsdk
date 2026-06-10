#pragma once

#include <nvk/math/Vec3.h>
#include <nvk/sim/RprEntityType.h>
#include <nvk_types.h>

namespace nv {

/// Static helpers that decode raw RPR-FOM 2.0 attribute byte buffers into
/// NervSDK types.
///
/// No dependency on OpenRTI or any HLA library — works with any byte source
/// (RTI VariableLengthData, a network buffer, a file, a unit test fixture).
///
/// All RPR-FOM 2.0 / HLA 1516e primitive fields are big-endian:
///   HLAfloat64BE  = IEEE 754 double, big-endian  (8 bytes)
///   HLAfloat32BE  = IEEE 754 float,  big-endian  (4 bytes)
///   HLAinteger16BE = uint16, big-endian           (2 bytes)
///   HLAoctet       = uint8                        (1 byte)
///
/// Wire layouts (offsets in bytes):
///
///   WorldLocationStruct  (24 bytes):
///     [0..7]   X : HLAfloat64BE   (ECEF X, metres)
///     [8..15]  Y : HLAfloat64BE   (ECEF Y, metres)
///     [16..23] Z : HLAfloat64BE   (ECEF Z, metres)
///
///   OrientationStruct  (12 bytes):
///     [0..3]   Psi   : HLAfloat32BE  (heading/yaw, radians)
///     [4..7]   Theta : HLAfloat32BE  (pitch,        radians)
///     [8..11]  Phi   : HLAfloat32BE  (roll,         radians)
///
///   EntityTypeStruct  (8 bytes):
///     [0]      EntityKind       : HLAoctet
///     [1]      Domain           : HLAoctet
///     [2..3]   CountryCode      : HLAinteger16BE
///     [4]      Category         : HLAoctet
///     [5]      Subcategory      : HLAoctet
///     [6]      Specific         : HLAoctet
///     [7]      Extra            : HLAoctet
///
///   EntityIdentifierStruct  (6 bytes):
///     [0..1]   SiteID           : HLAinteger16BE
///     [2..3]   ApplicationID    : HLAinteger16BE
///     [4..5]   EntityNumber     : HLAinteger16BE
struct RprDecoder {

    // -------------------------------------------------------------------------
    // Attribute decoders

    /// Decode WorldLocationStruct → ECEF XYZ in metres.
    /// @param buf  pointer to the first byte of the attribute value
    /// @param len  byte length of the buffer (must be >= 24)
    /// @return     Vec3d(X, Y, Z) in ECEF metres
    static auto decode_world_location(const U8* buf, size_t len) -> Vec3d;

    /// Decode OrientationStruct → (psi, theta, phi) in radians.
    /// @param buf  pointer to the first byte of the attribute value
    /// @param len  byte length of the buffer (must be >= 12)
    /// @return     Vec3f(psi, theta, phi) in radians
    static auto decode_orientation(const U8* buf, size_t len) -> Vec3f;

    /// Decode EntityTypeStruct → RprEntityType.
    /// @param buf  pointer to the first byte of the attribute value
    /// @param len  byte length of the buffer (must be >= 8)
    static auto decode_entity_type(const U8* buf, size_t len) -> RprEntityType;

    /// Decode EntityIdentifierStruct → a compact U64 key.
    /// Packing: (SiteID << 32) | (ApplicationID << 16) | EntityNumber.
    /// This key is stable and unique within a federation.
    /// @param buf  pointer to the first byte of the attribute value
    /// @param len  byte length of the buffer (must be >= 6)
    static auto decode_entity_id(const U8* buf, size_t len) -> U64;

    // -------------------------------------------------------------------------
    // Low-level big-endian primitives (public so they can be reused and tested)

    static auto read_f64_be(const U8* p) noexcept -> F64;
    static auto read_f32_be(const U8* p) noexcept -> F32;
    static auto read_u16_be(const U8* p) noexcept -> U16;
};

} // namespace nv
