#include <external/earcut.hpp>
#include <nvk/pcg/BuildingConstructor.h>

namespace nv {

auto BuildingConstructor::get_texture(const String& tname)
    -> const CellTextureDesc& {
    auto it = textures.find(tname);
    if (it != textures.end()) {
        return *it->second;
    }

    // Get the texture from the layout:
    const auto& desc =
        atlasLayout->pick_texture_desc("building", subtype, style, tname, id);
    textures.insert(std::make_pair(tname, &desc));
    return desc;
}

BuildingConstructor::BuildingConstructor(U64 bid, String stype,
                                         const CellTextureAtlasLayout& atlas)
    : id(bid), style(atlas.pick_style("building", stype, id)),
      atlasLayout(&atlas) {

    subtype = std::move(stype);
};

auto BuildingConstructor::create_facade(const Vec2d& a, const Vec2d& b,
                                        Vector<CellVertex>& vertices,
                                        Vector<U32>& indices) -> bool {

    auto edgeDir = b - a;
    auto edgeLen = edgeDir.length();

    if (edgeLen < 1.0)
        return false; // degenerate edge < 1 unit

    edgeDir = edgeDir / edgeLen;

    // Outward normal (CW 90° of forward direction = right-hand side of CCW
    // ring):
    const F32 nx = F32(edgeDir.y());
    const F32 ny = F32(-edgeDir.x());

    const auto pushVertex = [&](const Vec2d& p, F64 z, F64 u0, F64 v0,
                                const CellTextureDesc& tdesc) {
        CellVertex v{};
        v.px = F32(p.x() - origin.x());
        v.py = F32(p.y() - origin.y());
        v.pz = F32(z);
        v.nx = nx;
        v.ny = ny;
        v.nz = 0.F;
        // v.u0 = u0;
        // v.v0 = v0;
        // remap_uv_to_atlas(u0, v0, wallTex, v.u0, v.v0);
        tdesc.scale_uv(F32(u0), F32(v0), v.u0, v.v0);
        v.texIdx = F32(tdesc.index);
        vertices.push_back(v);
    };

    const auto pushQuad = [&](const Vec3d& bl, const Vec2d& xdir, F64 height,
                              const Vec2d& uv0, const CellTextureDesc& tdesc) {
        auto a = bl.xy();
        auto b = a + xdir;
        auto len = xdir.length();
        const U32 base = U32(vertices.size());

        pushVertex(a, bl.z(), uv0.x(), uv0.y(), tdesc); // bottom-left
        pushVertex(a, bl.z() + height, uv0.x(), uv0.y() + height * uvScale,
                   tdesc); // top-left

        pushVertex(b, bl.z(), uv0.x() + len * uvScale, uv0.y(),
                   tdesc); // bottom-right
        pushVertex(b, bl.z() + height, uv0.x() + len * uvScale,
                   uv0.y() + height * uvScale,
                   tdesc); // top-right

        // Indices: CCW winding viewed from outside (outward normal side).
        // Quad layout: BL=base+0, TL=base+1, BR=base+2, TR=base+3
        // Tri 1: BL, BR, TL
        indices.push_back(base + 0);
        indices.push_back(base + 2);
        indices.push_back(base + 1);
        // Tri 2: TL, BR, TR
        indices.push_back(base + 1);
        indices.push_back(base + 2);
        indices.push_back(base + 3);
    };

    // Here we should construct a wall facade:
    const auto& wallDesc = get_texture("wall");
    const auto& doorDesc = get_texture("door");
    const auto& windowDesc = get_texture("window");

    pushQuad({a, bottomHeight}, b - a, topHeight - bottomHeight, {0, 0},
             wallDesc);

    // // Push basement:
    // if(buriedHeight>0.0) {
    // }

    return true;
};
void BuildingConstructor::create_roof(const Vector<Vec2d>& ring,
                                      Vector<CellVertex>& vertices,
                                      Vector<U32>& indices) {
    // Triangulate the closed footprint ring at roofZ using earcut.
    // UV0: (world_X_m, world_Y_m) — planar projection, tile-local.
    using EarcutPoint = std::array<float, 2>;
    std::vector<std::vector<EarcutPoint>> polygon;
    const U32 n = U32(ring.size());

    auto& outerRing = polygon.emplace_back();
    outerRing.reserve(n);

    const auto& roofDesc = get_texture("roof");

    const U32 roofBase = U32(vertices.size());

    for (U32 i = 0; i < n; ++i) {
        const Vec2d& pt = ring[i];
        const F32 lx = F32(pt.x() - origin.x());
        const F32 ly = F32(pt.y() - origin.y());

        outerRing.push_back({lx, ly});

        CellVertex v{};
        v.px = lx;
        v.py = ly;
        v.pz = topHeight;
        v.nx = 0.f;
        v.ny = 0.f;
        v.nz = 1.f;
        // Planar UV in metres (world coords, not tile-local, to match
        // building_placer.py which uses world_X_m / world_Y_m).
        F32 u0 = (F32(pt.x()) / 100.0F);
        F32 v0 = (F32(pt.y()) / 100.0F);
        // v.u0 = u0;
        // v.v0 = v0;
        // remap_uv_to_atlas(u0, v0, roofTex, v.u0, v.v0);
        roofDesc.scale_uv(u0, v0, v.u0, v.v0);
        v.texIdx = F32(roofDesc.index);
        vertices.push_back(v);
    }

    // earcut returns indices into the ring (0-based); shift by roofBase.
    auto rawIdx = mapbox::earcut<U32>(polygon);

    // earcut produces CW winding in screen space (Y-down); UE uses Y-up
    // with CCW front-face convention.  Since the UE viewport has Y pointing
    // north (up in world), earcut's XY matches the UE XY plane and the
    // winding is already CCW when viewed from above (Z+).  Reverse each
    // triangle to match the expected outward (Z+) normal.
    for (U32 t = 0; t + 2 < U32(rawIdx.size()); t += 3) {
        indices.push_back(roofBase + rawIdx[t + 0]);
        indices.push_back(roofBase + rawIdx[t + 2]); // swap t+1/t+2
        indices.push_back(roofBase + rawIdx[t + 1]);
    }
};

} // namespace nv