#pragma once

#include <nvk_common.h>
#include <nvk_types.h>

namespace nv {

/// RPR-FOM 2.0 / DIS EntityType — 7-tuple uniquely identifying the kind of a
/// simulated entity. Matches IEEE 1278.1 (DIS) and SISO-STD-001 (RPR-FOM 2.0).
///
/// Encoding on the wire (HLA): 8 bytes, fields in order below.
///   kind(1) domain(1) countryCode(2BE) category(1) subcategory(1) specific(1)
///   extra(1)
///
/// Common EntityKind values:
///   0 = Other,  1 = Platform,  2 = Munition,  3 = LifeForm,
///   4 = Environmental,  5 = Cultural Feature,  6 = Supply,  7 = Radio
///
/// Common Domain values (for Platform):
///   0 = Other,  1 = Land,  2 = Air,  3 = Surface,  4 = Subsurface,  5 = Space
struct RprEntityType {
    U8 kind = 0;   ///< EntityKind enum (see DIS 7 / RPR-FOM 2.0)
    U8 domain = 0; ///< Domain enum
    U16 countryCode =
        0; ///< ISO 3166-1 numeric country code (big-endian on wire)
    U8 category = 0;
    U8 subcategory = 0;
    U8 specific = 0;
    U8 extra = 0;

    // -------------------------------------------------------------------------
    // Comparison

    [[nodiscard]] auto operator==(const RprEntityType& o) const noexcept
        -> bool {
        return kind == o.kind && domain == o.domain &&
               countryCode == o.countryCode && category == o.category &&
               subcategory == o.subcategory && specific == o.specific &&
               extra == o.extra;
    }
    [[nodiscard]] auto operator!=(const RprEntityType& o) const noexcept
        -> bool {
        return !(*this == o);
    }

    // -------------------------------------------------------------------------
    // Helpers

    /// Returns true when every field is zero (unknown / uninitialised type).
    [[nodiscard]] auto is_unknown() const noexcept -> bool {
        return kind == 0 && domain == 0 && countryCode == 0 && category == 0 &&
               subcategory == 0 && specific == 0 && extra == 0;
    }

    /// Compact dot-separated label, e.g. "1.1.222.1.1.0.0"
    /// (kind.domain.country.category.sub.specific.extra)
    [[nodiscard]] auto to_string() const -> String;

    /// Human-readable entity kind label, e.g. "Platform", "Munition".
    [[nodiscard]] auto kind_name() const -> const char*;

    /// Human-readable domain label, e.g. "Land", "Air", "Surface".
    /// Only meaningful when kind == 1 (Platform).
    [[nodiscard]] auto domain_name() const -> const char*;
};

} // namespace nv
