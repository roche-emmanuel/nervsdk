// File: nvk/io/GeoJsonDoc.cpp

#include <nvk/io/GeoJsonDoc.h>

namespace nv {

namespace {

auto type_from_name(const String& name) -> GeoGeomType {
    if (name == "Point") {
        return GeoGeomType::point;
    }
    if (name == "MultiPoint") {
        return GeoGeomType::multi_point;
    }
    if (name == "LineString") {
        return GeoGeomType::line_string;
    }
    if (name == "MultiLineString") {
        return GeoGeomType::multi_line_string;
    }
    if (name == "Polygon") {
        return GeoGeomType::polygon;
    }
    if (name == "MultiPolygon") {
        return GeoGeomType::multi_polygon;
    }
    return GeoGeomType::unknown;
}

/// A GeoJSON position is [x, y] with an optional third element we ignore.
auto read_position(const Json& jpos, Vec2d& out) -> bool {
    if (!jpos.is_array() || jpos.size() < 2) {
        return false;
    }
    if (!jpos[0].is_number() || !jpos[1].is_number()) {
        return false;
    }
    out.set(jpos[0].get<F64>(), jpos[1].get<F64>());
    return true;
}

auto read_ring(const Json& jring) -> Vector<Vec2d> {
    Vector<Vec2d> ring;
    if (!jring.is_array()) {
        return ring;
    }
    ring.reserve(jring.size());
    for (const Json& jpos : jring) {
        Vec2d pos;
        if (read_position(jpos, pos)) {
            ring.push_back(pos);
        }
    }
    return ring;
}

/// Signed area of a ring; positive counter-clockwise.
auto ring_signed_area(const Vector<Vec2d>& ring) -> F64 {
    if (ring.size() < 3) {
        return 0.0;
    }
    F64 area = 0.0;
    for (size_t i = 0; i < ring.size(); ++i) {
        const Vec2d& cur = ring[i];
        const Vec2d& nxt = ring[(i + 1) % ring.size()];
        area += cur.x() * nxt.y() - nxt.x() * cur.y();
    }
    return 0.5 * area;
}

} // namespace

auto geo_geom_type_name(GeoGeomType type) -> const char* {
    switch (type) {
    case GeoGeomType::point:
        return "Point";
    case GeoGeomType::multi_point:
        return "MultiPoint";
    case GeoGeomType::line_string:
        return "LineString";
    case GeoGeomType::multi_line_string:
        return "MultiLineString";
    case GeoGeomType::polygon:
        return "Polygon";
    case GeoGeomType::multi_polygon:
        return "MultiPolygon";
    case GeoGeomType::unknown:
        break;
    }
    return "unknown";
}

// ---------------------------------------------------------------------------
// GeoGeometry
// ---------------------------------------------------------------------------

auto GeoGeometry::all_points() const -> Vector<Vec2d> {
    Vector<Vec2d> out;
    size_t total = 0;
    for (const auto& ring : rings) {
        total += ring.size();
    }
    out.reserve(total);
    for (const auto& ring : rings) {
        out.insert(out.end(), ring.begin(), ring.end());
    }
    return out;
}

auto GeoGeometry::bounds() const -> Box2d {
    Box2d box;
    box.xmin = std::numeric_limits<F64>::max();
    box.ymin = std::numeric_limits<F64>::max();
    box.xmax = -std::numeric_limits<F64>::max();
    box.ymax = -std::numeric_limits<F64>::max();

    for (const auto& ring : rings) {
        for (const Vec2d& pos : ring) {
            box.xmin = std::min(box.xmin, pos.x());
            box.ymin = std::min(box.ymin, pos.y());
            box.xmax = std::max(box.xmax, pos.x());
            box.ymax = std::max(box.ymax, pos.y());
        }
    }

    return box;
}

auto GeoGeometry::length() const -> F64 {
    F64 total = 0.0;
    for (const auto& ring : rings) {
        for (size_t i = 1; i < ring.size(); ++i) {
            total += (ring[i] - ring[i - 1]).length();
        }
    }
    return total;
}

auto GeoGeometry::centroid() const -> Vec2d {
    if (rings.empty() || rings[0].empty()) {
        return {};
    }

    const Vector<Vec2d>& outer = rings[0];

    // Vertex mean, computed up front: it is the fallback for every
    // degenerate case below and costs nothing.
    Vec2d mean;
    for (const Vec2d& pos : outer) {
        mean += pos;
    }
    mean /= F64(outer.size());

    switch (type) {
    case GeoGeomType::point:
    case GeoGeomType::multi_point:
        return mean;

    case GeoGeomType::line_string:
    case GeoGeomType::multi_line_string: {
        // Midpoint by arclength, not by vertex index: a line with a dense
        // corner and a long straight run has its vertex mean sitting in the
        // corner, which is not where anyone would point at it.
        const F64 half = length() * 0.5;
        F64 walked = 0.0;
        for (const auto& ring : rings) {
            for (size_t i = 1; i < ring.size(); ++i) {
                const F64 seg = (ring[i] - ring[i - 1]).length();
                if (walked + seg >= half && seg > 1e-12) {
                    const F64 tpar = (half - walked) / seg;
                    return ring[i - 1] + (ring[i] - ring[i - 1]) * tpar;
                }
                walked += seg;
            }
        }
        return mean;
    }

    case GeoGeomType::polygon:
    case GeoGeomType::multi_polygon: {
        const F64 area = ring_signed_area(outer);
        if (std::abs(area) < 1e-14) {
            return mean;
        }

        F64 cxs = 0.0;
        F64 cys = 0.0;
        for (size_t i = 0; i < outer.size(); ++i) {
            const Vec2d& cur = outer[i];
            const Vec2d& nxt = outer[(i + 1) % outer.size()];
            const F64 cross = cur.x() * nxt.y() - nxt.x() * cur.y();
            cxs += (cur.x() + nxt.x()) * cross;
            cys += (cur.y() + nxt.y()) * cross;
        }

        return {cxs / (6.0 * area), cys / (6.0 * area)};
    }

    case GeoGeomType::unknown:
        break;
    }

    return mean;
}

// ---------------------------------------------------------------------------
// GeoFeature
// ---------------------------------------------------------------------------

auto GeoFeature::str(const String& key, const String& fallback) const
    -> String {
    if (!has(key)) {
        return fallback;
    }
    const Json& val = props.at(key);
    if (val.is_string()) {
        return val.get<String>();
    }
    if (val.is_number() || val.is_boolean()) {
        return val.dump();
    }
    return fallback;
}

auto GeoFeature::num(const String& key, F64 fallback) const -> F64 {
    if (!has(key)) {
        return fallback;
    }
    const Json& val = props.at(key);
    if (val.is_number()) {
        return val.get<F64>();
    }
    // Numbers routinely arrive quoted out of GIS exports, so parse strings
    // rather than silently returning the fallback.
    if (val.is_string()) {
        try {
            return std::stod(val.get<String>());
        } catch (const std::exception&) {
            return fallback;
        }
    }
    return fallback;
}

auto GeoFeature::integer(const String& key, I32 fallback) const -> I32 {
    return I32(std::llround(num(key, F64(fallback))));
}

auto GeoFeature::flag(const String& key, bool fallback) const -> bool {
    if (!has(key)) {
        return fallback;
    }
    const Json& val = props.at(key);
    if (val.is_boolean()) {
        return val.get<bool>();
    }
    if (val.is_number()) {
        return val.get<F64>() != 0.0;
    }
    if (val.is_string()) {
        const String txt = val.get<String>();
        return txt == "true" || txt == "yes" || txt == "1";
    }
    return fallback;
}

// ---------------------------------------------------------------------------
// GeoJsonDoc
// ---------------------------------------------------------------------------

namespace {

auto parse_geometry(const Json& jgeom) -> GeoGeometry {
    GeoGeometry geom;

    if (!jgeom.is_object() || !jgeom.contains("type")) {
        return geom;
    }

    geom.type = type_from_name(jgeom.at("type").get<String>());
    if (geom.type == GeoGeomType::unknown || !jgeom.contains("coordinates")) {
        return geom;
    }

    const Json& coords = jgeom.at("coordinates");

    switch (geom.type) {
    case GeoGeomType::point: {
        Vec2d pos;
        if (read_position(coords, pos)) {
            geom.rings.push_back({pos});
        }
        break;
    }

    case GeoGeomType::multi_point:
    case GeoGeomType::line_string: {
        Vector<Vec2d> ring = read_ring(coords);
        if (!ring.empty()) {
            geom.rings.push_back(std::move(ring));
        }
        break;
    }

    case GeoGeomType::multi_line_string:
    case GeoGeomType::polygon: {
        if (coords.is_array()) {
            for (const Json& jring : coords) {
                Vector<Vec2d> ring = read_ring(jring);
                if (!ring.empty()) {
                    geom.rings.push_back(std::move(ring));
                }
            }
        }
        break;
    }

    case GeoGeomType::multi_polygon: {
        if (coords.is_array()) {
            geom.partFirst.push_back(0);
            for (const Json& jpoly : coords) {
                if (!jpoly.is_array()) {
                    continue;
                }
                for (const Json& jring : jpoly) {
                    Vector<Vec2d> ring = read_ring(jring);
                    if (!ring.empty()) {
                        geom.rings.push_back(std::move(ring));
                    }
                }
                geom.partFirst.push_back(U32(geom.rings.size()));
            }
        }
        break;
    }

    case GeoGeomType::unknown:
        break;
    }

    if (geom.partFirst.empty() && !geom.rings.empty()) {
        geom.partFirst = {0U, U32(geom.rings.size())};
    }

    return geom;
}

auto parse_feature(const Json& jfeat, U32 index) -> GeoFeature {
    GeoFeature feat;

    if (jfeat.contains("id")) {
        const Json& jid = jfeat.at("id");
        feat.id = jid.is_string() ? jid.get<String>() : jid.dump();
    }

    if (jfeat.contains("properties") && jfeat.at("properties").is_object()) {
        feat.props = jfeat.at("properties");
    } else {
        feat.props = Json::object();
    }

    if (jfeat.contains("geometry")) {
        feat.geom = parse_geometry(jfeat.at("geometry"));
    }

    // An unnamed feature still needs a stable identity: every downstream
    // consumer keys sites by id, and "the third one in the file" is a much
    // better answer than an empty string shared by twenty others.
    if (feat.id.empty()) {
        feat.id = feat.str("id", feat.str("name", fmt::format("f{}", index)));
    }

    return feat;
}

} // namespace

auto GeoJsonDoc::parse(const Json& root) -> GeoJsonDoc {
    GeoJsonDoc doc;

    if (!root.is_object()) {
        logWARN("GeoJsonDoc: root is not an object");
        return doc;
    }

    const String type = root.value("type", String{});

    if (type == "FeatureCollection") {
        if (!root.contains("features") || !root.at("features").is_array()) {
            logWARN("GeoJsonDoc: FeatureCollection has no 'features' array");
            return doc;
        }
        const Json& feats = root.at("features");
        doc.features.reserve(feats.size());
        U32 index = 0;
        for (const Json& jfeat : feats) {
            GeoFeature feat = parse_feature(jfeat, index++);
            if (feat.geom.is_valid()) {
                doc.features.push_back(std::move(feat));
            }
        }
        return doc;
    }

    if (type == "Feature") {
        GeoFeature feat = parse_feature(root, 0);
        if (feat.geom.is_valid()) {
            doc.features.push_back(std::move(feat));
        }
        return doc;
    }

    // A bare geometry. Legal, and occasionally what a quick hand-edit
    // produces, so accept it rather than making the user wrap it.
    GeoFeature feat;
    feat.geom = parse_geometry(root);
    feat.props = Json::object();
    feat.id = "f0";
    if (feat.geom.is_valid()) {
        doc.features.push_back(std::move(feat));
    } else {
        logWARN("GeoJsonDoc: root type '{}' is not a FeatureCollection, a "
                "Feature or a geometry",
                type);
    }

    return doc;
}

auto GeoJsonDoc::read_file(const String& path) -> GeoJsonDoc {
    NVCHK(system_file_exists(path), "GeoJsonDoc: '{}' does not exist", path);

    const Json root = read_json_file(path, /*forceAllowSystem=*/true);
    GeoJsonDoc doc = parse(root);

    logINFO("GeoJsonDoc: read '{}' — {}", path, doc.describe());
    return doc;
}

auto GeoJsonDoc::where(const String& key, const String& value) const
    -> Vector<const GeoFeature*> {
    Vector<const GeoFeature*> out;
    for (const GeoFeature& feat : features) {
        if (feat.str(key) == value) {
            out.push_back(&feat);
        }
    }
    return out;
}

auto GeoJsonDoc::describe() const -> String {
    U32 counts[7] = {0, 0, 0, 0, 0, 0, 0};
    for (const GeoFeature& feat : features) {
        counts[U32(feat.geom.type)]++;
    }

    return fmt::format("{} feature(s): {} point, {} line, {} polygon, {} "
                       "multi",
                       features.size(), counts[U32(GeoGeomType::point)],
                       counts[U32(GeoGeomType::line_string)],
                       counts[U32(GeoGeomType::polygon)],
                       counts[U32(GeoGeomType::multi_point)] +
                           counts[U32(GeoGeomType::multi_line_string)] +
                           counts[U32(GeoGeomType::multi_polygon)]);
}

} // namespace nv
