#include <nvk/geometry/Region2.h>

#include <clipper2/clipper.h>

namespace nv {

namespace {
auto to_paths(const Vector<Polygon2d>& polys) -> Clipper2Lib::PathsD {
    Clipper2Lib::PathsD paths;
    paths.reserve(polys.size());
    for (const auto& poly : polys) {
        Clipper2Lib::PathD path;
        path.reserve(poly.coords.size());
        for (const auto& pt : poly.coords) {
            path.emplace_back(pt.x(), pt.y());
        }
        paths.push_back(std::move(path));
    }
    return paths;
}

auto to_polygon(const Clipper2Lib::PathD& path, bool positive) -> Polygon2d {
    Polygon2d poly;
    poly.coords.reserve(path.size());
    for (const auto& pt : path) {
        poly.coords.emplace_back(pt.x, pt.y);
    }
    // Normalise whatever winding the tree gives: outers > 0, holes < 0.
    if ((polygon2_area(poly) >= 0.0) != positive) {
        std::reverse(poly.coords.begin(), poly.coords.end());
    }
    return poly;
}

// `node` is an outer ring (even depth); its children are its holes, and their
// own children are the islands inside those holes, outers again.
void collect(const Clipper2Lib::PolyPathD& node, Vector<Region2d>& out) {
    Region2d region;
    region.outer = to_polygon(node.Polygon(), true);
    for (size_t i = 0; i < node.Count(); ++i) {
        const auto* hole = node.Child(i);
        region.holes.push_back(to_polygon(hole->Polygon(), false));
    }
    if (region.outer.coords.size() >= 3) {
        out.push_back(std::move(region));
    }
    for (size_t i = 0; i < node.Count(); ++i) {
        const auto* hole = node.Child(i);
        for (size_t j = 0; j < hole->Count(); ++j) {
            collect(*hole->Child(j), out);
        }
    }
}
} // namespace

auto polygon2_regions(Polygon2Op op, const Vector<Polygon2d>& subjects,
                      const Vector<Polygon2d>& clips, I32 fillRule)
    -> Vector<Region2d> {
    Clipper2Lib::ClipperD clipper(2); // same precision as the other polygon2_*
    clipper.AddSubject(to_paths(subjects));
    if (op != Polygon2Op::Union && !clips.empty()) {
        clipper.AddClip(to_paths(clips));
    }

    Clipper2Lib::ClipType type = Clipper2Lib::ClipType::Union;
    switch (op) {
    case Polygon2Op::Union:
        type = Clipper2Lib::ClipType::Union;
        break;
    case Polygon2Op::Intersection:
        type = Clipper2Lib::ClipType::Intersection;
        break;
    case Polygon2Op::Difference:
        type = Clipper2Lib::ClipType::Difference;
        break;
    case Polygon2Op::Xor:
        type = Clipper2Lib::ClipType::Xor;
        break;
    }

    Clipper2Lib::PolyTreeD tree;
    Clipper2Lib::PathsD openPaths;
    clipper.Execute(type, (Clipper2Lib::FillRule)fillRule, tree, openPaths);

    Vector<Region2d> out;
    for (size_t i = 0; i < tree.Count(); ++i) {
        collect(*tree.Child(i), out);
    }
    return out;
}

auto region2_rings(const Region2d& region) -> Vector<Polygon2d> {
    Vector<Polygon2d> rings;
    rings.reserve(region.holes.size() + 1);
    rings.push_back(region.outer);
    rings.insert(rings.end(), region.holes.begin(), region.holes.end());
    return rings;
}

auto region2_area(const Region2d& region) -> F64 {
    F64 area = std::abs(polygon2_area(region.outer));
    for (const auto& hole : region.holes) {
        area -= std::abs(polygon2_area(hole));
    }
    return area;
}

} // namespace nv
