// file: sources/nvk/base/uuid.h

#ifndef NV_UUID_H_
#define NV_UUID_H_

#include <nvk/base/std_containers.h>
#include <nvk_base.h>

namespace nv {

/** A 128-bit universally unique identifier, RFC 4122 compatible.

    The Uuid is stored as 16 raw bytes and exposes conversions to and from
    the canonical 8-4-4-4-12 hex string format
    ("550e8400-e29b-41d4-a716-446655440000"). It is value-typed, cheap to
    copy, hashable, and can be used as a map key.

    Generation:
      - generate_v4() produces a random version-4 UUID using the engine's
        RandGen as the entropy source. The result has the version (4) and
        variant (RFC 4122) bits set per the spec.
      - generate_v4_string() is a convenience that returns the canonical
        string form directly, for callers that just want a unique string
        ID and never touch the binary form.

    The 'nil' UUID (all zero bytes) compares equal to any other nil UUID
    and is the default-constructed value. Use is_nil() to check. */
class Uuid {
  public:
    static constexpr U32 BYTE_COUNT = 16;
    static constexpr U32 STRING_LENGTH =
        36; // "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"

    /** Construct a nil UUID (all zero bytes). */
    Uuid();

    /** Construct from 16 raw bytes. */
    explicit Uuid(const U8 (&bytes)[BYTE_COUNT]);

    /** Parse from a canonical hex string. Throws on malformed input.
        Accepts both lowercase and uppercase hex digits. The hyphens at
        positions 8, 13, 18, 23 are required. */
    static auto from_string(const String& str) -> Uuid;

    /** Parse from a canonical hex string. Returns true on success and
        writes the result to 'out'; returns false without throwing on
        malformed input. Useful for trust-boundary parsing. */
    static auto try_from_string(const String& str, Uuid& out) -> bool;

    /** Generate a random version-4 UUID. */
    static auto generate_v4() -> Uuid;

    /** Generate a random version-4 UUID and return its canonical string
        form directly. Equivalent to generate_v4().to_string() but skips
        the intermediate object for the common "I just want a unique
        string ID" use case. */
    static auto generate_v4_string() -> String;

    /** Convert to canonical hex string
     * ("xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"). */
    [[nodiscard]] auto to_string() const -> String;

    /** Raw byte access. */
    [[nodiscard]] auto bytes() const -> const U8* { return _bytes.data(); }

    /** True if every byte is zero (the nil UUID). */
    [[nodiscard]] auto is_nil() const -> bool;

    auto operator==(const Uuid& rhs) const -> bool {
        return _bytes == rhs._bytes;
    }
    auto operator!=(const Uuid& rhs) const -> bool { return !(*this == rhs); }
    auto operator<(const Uuid& rhs) const -> bool {
        return _bytes < rhs._bytes;
    }

  private:
    std::array<U8, BYTE_COUNT> _bytes{};
};

} // namespace nv

namespace std {

template <> struct hash<nv::Uuid> {
    auto operator()(const nv::Uuid& uuid) const noexcept -> std::size_t {
        const nv::U8* p = uuid.bytes();

        // FNV-1a constants sized to the platform's size_t.
        // 64-bit: offset=14695981039346656037, prime=1099511628211
        // 32-bit: offset=2166136261,           prime=16777619
        if constexpr (sizeof(std::size_t) == 8) {
            std::size_t h = 14695981039346656037ULL;
            for (nv::U32 i = 0; i < nv::Uuid::BYTE_COUNT; ++i) {
                h ^= static_cast<std::size_t>(p[i]);
                h *= 1099511628211ULL;
            }
            return h;
        } else {
            std::size_t h = 2166136261UL;
            for (nv::U32 i = 0; i < nv::Uuid::BYTE_COUNT; ++i) {
                h ^= static_cast<std::size_t>(p[i]);
                h *= 16777619UL;
            }
            return h;
        }
    }
};

// Add support to write to string:
inline auto operator<<(std::ostream& os, const nv::Uuid& uid) -> std::ostream& {
    os << uid.to_string();
    return os;
}

} // namespace std

template <> struct fmt::formatter<nv::Uuid> {
    constexpr auto parse(format_parse_context& ctx) const
        -> decltype(ctx.begin()) {
        const auto* it = ctx.begin();
        const auto* end = ctx.end();

        // Check if reached the end of the range:
        if (it != end && *it != '}')
            throw format_error("invalid format");

        // Return an iterator past the end of the parsed range:
        return it;
    }

    // Overloaded format method
    auto format(const nv::Uuid uid, fmt::format_context& ctx) const
        -> decltype(ctx.out()) {
        return fmt::format_to(ctx.out(), "{}", uid.to_string());
    }
};

#endif