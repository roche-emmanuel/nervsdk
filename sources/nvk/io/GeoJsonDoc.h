// File: nvk/io/GeoJsonDoc.h
//
// A small, forgiving GeoJSON reader.
//
// Just enough of RFC 7946 to consume hand-authored airport and area-of-
// interest files: FeatureCollection, Feature, and the six geometry types that
// carry coordinates. No CRS handling, no bounding boxes, no foreign members
// beyond `properties`, which is handed back as raw Json for the caller to
// interpret however its schema demands.
//
// Coordinates come out exactly as they went in — (x, y) pairs, which for a
// WGS84 file means (longitude, latitude) in that order, per the spec.
// Projecting them is the caller's job, and deliberately so: nvk has no
// business knowing about the world's origin.
//
// The third coordinate
// --------------------
// RFC 7946 allows a position to carry a third element, and reads it as an
// altitude above the WGS84 ellipsoid. Almost nobody authoring by hand means
// that: KML calls the same slot `relativeToGround` or `absolute` depending on
// an explicit mode property, and hand-drawn route files follow suit. So this
// reader stores the value without interpreting it, in `ringsZ`, and leaves
// the meaning entirely to the caller's own `altitude_mode` property.
//
// `ringsZ` is either empty (no position in the geometry carried a Z) or
// exactly parallel to `rings`. Never partially filled: a geometry where only
// some positions have a third element gets 0.0 for the rest, because a
// caller indexing in parallel must never have to bounds-check.

#ifndef _NV_GEOJSONDOC_H_
#define _NV_GEOJSONDOC_H_

#include <nvk/math/Box2.h>
#include <nvk/math/Vec2.h>
#include <nvk_common.h>
#include <nvk_types.h>

namespace nv {

enum class GeoGeomType : U8 {
    unknown = 0,
    point,
    multi_point,
    line_string,
    multi_line_string,
    polygon,
    multi_polygon,
};

[[nodiscard]] auto geo_geom_type_name(GeoGeomType type) -> const char*;

/// One geometry, flattened.
///
/// Every type is stored as a list of coordinate rings, which collapses six
/// cases into one at the cost of a little precision in the type system:
///   * point / multi_point   -> one ring holding the point(s)
///   * line_string           -> one ring
///   * multi_line_string     -> one ring per line
///   * polygon               -> ring 0 is the outer boundary, the rest holes
///   * multi_polygon         -> every polygon's rings concatenated, with
///                              `partFirst` marking where each starts
struct GeoGeometry {
    GeoGeomType type{GeoGeomType::unknown};

    Vector<Vector<Vec2d>> rings;

    /// Third coordinate per position, same shape as `rings`. Empty when no
    /// position in the geometry carried one — which is the overwhelmingly
    /// common case, so consumers that do not care pay nothing.
    Vector<Vector<F64>> ringsZ;

    /// Index into `rings` of the first ring of each part. Size is the number
    /// of parts + 1, CSR style. For everything except multi_polygon this is
    /// simply {0, rings.size()}.
    Vector<U32> partFirst;

    [[nodiscard]] auto num_parts() const -> U32 {
        return partFirst.size() >= 2 ? U32(partFirst.size()) - 1 : 0;
    }

    [[nodiscard]] auto is_valid() const -> bool {
        return type != GeoGeomType::unknown && !rings.empty();
    }

    [[nodiscard]] auto has_z() const -> bool { return !ringsZ.empty(); }

    /// Third coordinate of one position, or 0.0 when the geometry carries
    /// none. Out-of-range indices also read 0.0 rather than asserting: a
    /// caller walking `rings` in parallel should not need a second bounds
    /// check for the optional channel.
    [[nodiscard]] auto z_at(U32 ringIdx, U32 ptIdx) const -> F64;

    /// Every coordinate, in order. Handy for a bounding box or a centroid.
    [[nodiscard]] auto all_points() const -> Vector<Vec2d>;

    [[nodiscard]] auto bounds() const -> Box2d;

    /// Area-weighted centroid of the outer ring for a polygon, midpoint of
    /// the arclength for a line, the point itself for a point. Falls back to
    /// the vertex mean whenever the shape is degenerate.
    [[nodiscard]] auto centroid() const -> Vec2d;

    /// Total planar length of every ring (degrees, or whatever the input
    /// units are — the caller projects before caring).
    [[nodiscard]] auto length() const -> F64;
};

/// One feature: a geometry plus its property bag.
struct GeoFeature {
    String id;
    GeoGeometry geom;
    Json props;

    [[nodiscard]] auto has(const String& key) const -> bool {
        return props.is_object() && props.contains(key);
    }

    [[nodiscard]] auto str(const String& key, const String& fallback = {}) const
        -> String;

    [[nodiscard]] auto num(const String& key, F64 fallback = 0.0) const -> F64;

    [[nodiscard]] auto integer(const String& key, I32 fallback = 0) const
        -> I32;

    [[nodiscard]] auto flag(const String& key, bool fallback = false) const
        -> bool;
};

/// A parsed document.
struct GeoJsonDoc {
    Vector<GeoFeature> features;

    [[nodiscard]] static auto read_file(const String& path) -> GeoJsonDoc;
    [[nodiscard]] static auto parse(const Json& root) -> GeoJsonDoc;

    /// Features whose property `key` equals `value`. Pointers into
    /// `features`, so they die with the document.
    [[nodiscard]] auto where(const String& key, const String& value) const
        -> Vector<const GeoFeature*>;

    [[nodiscard]] auto empty() const -> bool { return features.empty(); }

    /// One-line summary for the load log: counts by geometry type.
    [[nodiscard]] auto describe() const -> String;
};

} // namespace nv

#endif