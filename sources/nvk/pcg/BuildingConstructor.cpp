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

    // Whole facade:
    // pushQuad({a, bottomHeight}, b - a, topHeight - bottomHeight, {0, 0},
    //          wallDesc);

    F64 remainingHeight = topHeight - bottomHeight;

    // Push basement:
    if (buriedHeight > 0.0) {
        pushQuad({a, bottomHeight}, b - a, buriedHeight, {0, 0}, wallDesc);
        remainingHeight -= buriedHeight;
    }

    // Compute the number of floors:
    U32 nfloors = std::max(1U, (U32)round(remainingHeight / levelHeight));

    // Actual floor height:
    F64 actualFloorHeight = remainingHeight / nfloors;

    const auto& rng = RandGen::instance();

    F64 edgeLenM = edgeLen * uvScale;
    // Compute number of doors to add:
    I32 nDoors = (actualFloorHeight < doorDesc.dimsM.y() ||
                  edgeLenM < doorDesc.dimsM.x() || numPlacedDoors > 0)
                     ? 0
                     : 1;

    F64 doorWidthM = doorDesc.dimsM.x();
    F64 doorHeightM = doorDesc.dimsM.y();
    F64 windowWidthM = windowDesc.dimsM.x();
    F64 windowHeightM = windowDesc.dimsM.x();
    F64 winBaseM = 1.0;

    // Check how many windows we could fit on this facade:
    I32 maxNWindows = actualFloorHeight < (windowDesc.dimsM.y() + winBaseM)
                          ? 0U
                          : (I32)std::floor((edgeLenM) / windowDesc.dimsM.x());
    I32 maxNWindows0 = actualFloorHeight < (windowDesc.dimsM.y() + winBaseM)
                           ? 0U
                           : (I32)std::floor((edgeLenM - nDoors * doorWidthM) /
                                             windowDesc.dimsM.x());

    I32 nWindows = rng.uniform_int(0, maxNWindows);
    I32 nWindows0 = rng.uniform_int(0, maxNWindows0);

    I32 nElems0 = nDoors + nWindows0;
    I32 doorIdx = rng.uniform_int(0, nElems0 - 1);

    F64 currentHeight = bottomHeight + buriedHeight;

    auto buildFloor = [&](I32 nDoors, I32 nWindows, Vec3d pos) {
        // First floor
        F64 usedLenM = nDoors * doorWidthM + nWindows * windowWidthM;
        I32 nelems = nDoors + nWindows;
        F64 flen =
            nelems > 0.0 ? (edgeLenM - usedLenM) / (nelems + 1) : edgeLenM;
        Vec2d uv0 = {0.0, (currentHeight - bottomHeight) * uvScale};

        // Place a first filler:
        pushQuad(pos, edgeDir * flen / uvScale, actualFloorHeight, uv0,
                 wallDesc);
        uv0.x() += flen;
        pos += Vec3d(edgeDir * flen / uvScale, 0.0);

        for (I32 j = 0; j < nelems; ++j) {

            F64 ww = windowWidthM / uvScale;
            F64 hh = windowHeightM / uvScale;

            if (j == doorIdx) {
                // Place a door:
                ww = doorWidthM / uvScale;
                hh = doorHeightM / uvScale;

                pushQuad(pos, edgeDir * ww, hh, {0, 0}, doorDesc);
                // Place filler above door:
                pushQuad(pos + Vec3d(0, 0, hh), edgeDir * ww,
                         actualFloorHeight - hh, uv0 + Vec2d(0.0, doorHeightM),
                         wallDesc);

            } else {
                // filler below:
                pushQuad(pos, edgeDir * ww, winBaseM, uv0, wallDesc);
                // Window:
                pushQuad(pos + Vec3d(0, 0, winBaseM), edgeDir * ww, hh, {0, 0},
                         windowDesc);
                pushQuad(pos + Vec3d(0, 0, winBaseM + windowHeightM),
                         edgeDir * ww,
                         actualFloorHeight - hh - winBaseM / uvScale,
                         uv0 + Vec2d(0, winBaseM + windowHeightM), wallDesc);
            }

            uv0.x() += flen;
            pos += Vec3d(edgeDir * flen / uvScale, 0.0);

            // Place filler:
            pushQuad(pos, edgeDir * flen / uvScale, actualFloorHeight, uv0,
                     wallDesc);
        }
    };

    for (I32 fi = 0; fi < nfloors; ++fi) {
        auto pos = Vec3d(a, currentHeight);

        // Build one floor:
        if (fi == 0) {
            buildFloor(nDoors, nWindows0, pos);
        } else {
            // Other floors:
            buildFloor(0, nWindows, pos);
        }

        currentHeight += actualFloorHeight;
    }

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