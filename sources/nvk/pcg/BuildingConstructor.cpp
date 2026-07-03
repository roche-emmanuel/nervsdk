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
                                         const CellTextureAtlasLayout& atlas,
                                         TileGeom* out)
    : id(bid), style(atlas.pick_style("building", stype, id)), geom(out),
      atlasLayout(&atlas) {
    NVCHK(out != nullptr, "Invalid tile geom.");
    subtype = std::move(stype);
};

auto BuildingConstructor::create_facade(const Vec2d& a, const Vec2d& b)
    -> bool {

    Vec2d edgeDir = b - a;
    const F64 edgeLen = edgeDir.length();
    if (edgeLen < 1.0)
        return false; // degenerate edge < 1 cm

    edgeDir = edgeDir / edgeLen;

    // Outward normal (CW 90° of the forward direction = right-hand side of a
    // CCW ring):
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
        tdesc.scale_uv(F32(u0), F32(v0), v.u0, v.v0);
        v.texIdx = F32(tdesc.index);
        geom->verts.push_back(v);
    };

    // bl = bottom-left corner (world cm, z absolute); xdir = along-wall vector
    // (cm); height = cm; uv0 = tiling UV origin in *metres* (scale_uv divides
    // by dimsM). An element passes uv0={0,0} and sizes the quad to dimsM so it
    // maps exactly one 0..1 tile; a filler passes the running metre offset so
    // the wall texture tiles continuously.
    const auto pushQuad = [&](const Vec3d& bl, const Vec2d& xdir, F64 height,
                              const Vec2d& uv0, const CellTextureDesc& tdesc) {
        const Vec2d p0 = bl.xy();
        const Vec2d p1 = p0 + xdir;
        const F64 len = xdir.length();
        const U32 base = U32(geom->verts.size());

        pushVertex(p0, bl.z(), uv0.x(), uv0.y(), tdesc); // bottom-left
        pushVertex(p0, bl.z() + height, uv0.x(),         // top-left
                   uv0.y() + height * uvScale, tdesc);
        pushVertex(p1, bl.z(), uv0.x() + len * uvScale, uv0.y(), // bottom-right
                   tdesc);
        pushVertex(p1, bl.z() + height, uv0.x() + len * uvScale, // top-right
                   uv0.y() + height * uvScale, tdesc);

        // CCW winding viewed from outside (BL, TL, BR, TR = base 0,1,2,3):
        geom->indices.push_back(base + 0);
        geom->indices.push_back(base + 2);
        geom->indices.push_back(base + 1);
        geom->indices.push_back(base + 1);
        geom->indices.push_back(base + 2);
        geom->indices.push_back(base + 3);
    };

    const auto& wallDesc = get_texture("wall");
    const auto& doorDesc = get_texture("door");
    const auto& windowDesc = get_texture("window");

    // World XY (cm) at along-wall offset xM (metres) from `a`.
    const auto xyAt = [&](F64 xM) -> Vec2d {
        return a + edgeDir * (xM / uvScale);
    };

    // One wall-filler quad with continuous tiling UVs. widthM in metres,
    // zBottomCm/heightCm in cm.
    const auto emitFiller = [&](F64 xM, F64 zBottomCm, F64 widthM,
                                F64 heightCm) {
        if (widthM <= 1e-4 || heightCm <= 1e-2)
            return; // skip degenerate slivers
        const Vec2d uv0 = {xM, (zBottomCm - bottomHeight) * uvScale};
        pushQuad({xyAt(xM), zBottomCm}, edgeDir * (widthM / uvScale), heightCm,
                 uv0, wallDesc);
    };

    // One element (door/window) drawn as a single 0..1 tile at its physical
    // size. zBottomCm in cm.
    const auto emitElement = [&](F64 xM, F64 zBottomCm,
                                 const CellTextureDesc& desc) {
        pushQuad({xyAt(xM), zBottomCm}, edgeDir * (desc.dimsM.x() / uvScale),
                 desc.dimsM.y() / uvScale, {0, 0}, desc);
    };

    const F64 edgeLenM = edgeLen * uvScale;

    // ── Skirt (below the finished-floor elevation): one plain wall band ─────
    const F64 skirtCm = std::max(0.0, buriedHeight - bottomHeight);
    if (skirtCm > 1.0)
        emitFiller(0.0, bottomHeight, edgeLenM, skirtCm);

    // ── Floor grid spans [buriedHeight (ffe) .. topHeight] ─────────────────
    const F64 archHeightCm = std::max(0.0, topHeight - buriedHeight);
    // levelHeight is expected in cm (same convention as the original).
    const U32 nfloors =
        std::max(1U, U32(std::llround(archHeightCm / levelHeight)));
    const F64 floorHeightCm = archHeightCm / F64(nfloors);
    const F64 floorHeightM = floorHeightCm * uvScale;

    const F64 doorWidthM = doorDesc.dimsM.x();
    const F64 doorHeightM = doorDesc.dimsM.y();
    const F64 winWidthM = windowDesc.dimsM.x();
    const F64 winHeightM = windowDesc.dimsM.y(); // was .x() — height bug
    const F64 sillM = 1.0;    // window sill height above the floor line
    const F64 minPierM = 0.5; // minimum wall gap between/around openings

    const bool doorFits =
        (doorWidthM < edgeLenM) && (doorHeightM <= floorHeightM);
    const bool windowFits =
        (winWidthM < edgeLenM) && (sillM + winHeightM <= floorHeightM);

    // One door per building, on the ground floor of the first edge that fits.
    const I32 nDoors = (doorFits && numPlacedDoors == 0) ? 1 : 0;

    // How many windows fit on a run of length edgeLenM after reserving
    // reservedM, keeping nelems+1 piers each >= minPierM.
    const auto windowBudget = [&](F64 reservedM) -> I32 {
        if (!windowFits)
            return 0;
        const F64 num = edgeLenM - reservedM - minPierM;
        const F64 den = winWidthM + minPierM;
        if (num <= 0.0 || den <= 0.0)
            return 0;
        return std::max(0, I32(std::floor(num / den)));
    };

    const auto& rng = RandGen::instance();
    const I32 maxWinGround = windowBudget(nDoors * doorWidthM);
    const I32 maxWinUpper = windowBudget(0.0);
    const I32 nWinGround =
        maxWinGround > 0 ? rng.uniform_int(std::max(0, I32(maxWinGround * 0.7)),
                                           maxWinGround)
                         : 0;
    const I32 nWinUpper =
        maxWinUpper > 0
            ? rng.uniform_int(std::max(0, I32(maxWinGround * 0.7)), maxWinUpper)
            : 0;

    const I32 groundElems = nDoors + nWinGround;
    const I32 doorSlot = (nDoors > 0 && groundElems > 0)
                             ? rng.uniform_int(0, groundElems - 1)
                             : -1;

    // Lay out one floor: piers evenly distributed, elements sized to dimsM,
    // wall fillers above/under each opening.
    const auto buildFloor = [&](F64 floorBaseZ, I32 nDoorsF, I32 nWindowsF,
                                I32 doorSlotF) {
        const I32 nelems = nDoorsF + nWindowsF;
        const F64 usedM = nDoorsF * doorWidthM + nWindowsF * winWidthM;
        const F64 pierM =
            nelems > 0 ? (edgeLenM - usedM) / F64(nelems + 1) : edgeLenM;

        if (pierM < 0.0) { // openings don't fit — fall back to a plain wall
            emitFiller(0.0, floorBaseZ, edgeLenM, floorHeightCm);
            return;
        }

        F64 xM = 0.0;
        emitFiller(xM, floorBaseZ, pierM, floorHeightCm); // leading pier
        xM += pierM;

        for (I32 j = 0; j < nelems; ++j) {
            const bool isDoor = (nDoorsF > 0) && (j == doorSlotF);

            if (isDoor) {
                emitElement(xM, floorBaseZ, doorDesc);
                const F64 doorTopCm = floorBaseZ + doorHeightM / uvScale;
                emitFiller(xM, doorTopCm, doorWidthM,
                           floorBaseZ + floorHeightCm - doorTopCm);
                xM += doorWidthM;
            } else {
                const F64 sillTopCm = floorBaseZ + sillM / uvScale;
                const F64 winTopCm = sillTopCm + winHeightM / uvScale;
                emitFiller(xM, floorBaseZ, winWidthM, sillTopCm - floorBaseZ);
                emitElement(xM, sillTopCm, windowDesc);
                emitFiller(xM, winTopCm, winWidthM,
                           floorBaseZ + floorHeightCm - winTopCm);
                xM += winWidthM;
            }

            emitFiller(xM, floorBaseZ, pierM, floorHeightCm); // trailing pier
            xM += pierM;
        }
    };

    // Ground floor carries the door; every upper floor reuses one pattern.
    F64 floorBaseZ = buriedHeight;
    for (U32 fi = 0; fi < nfloors; ++fi) {
        if (fi == 0)
            buildFloor(floorBaseZ, nDoors, nWinGround, doorSlot);
        else
            buildFloor(floorBaseZ, 0, nWinUpper, -1);
        floorBaseZ += floorHeightCm;
    }

    if (nDoors > 0)
        ++numPlacedDoors;

    return true;
};

void BuildingConstructor::create_roof(const Vector<Vec2d>& ring) {
    // Triangulate the closed footprint ring at roofZ using earcut.
    // UV0: (world_X_m, world_Y_m) — planar projection, tile-local.
    using EarcutPoint = std::array<float, 2>;
    std::vector<std::vector<EarcutPoint>> polygon;
    const U32 n = U32(ring.size());

    auto& outerRing = polygon.emplace_back();
    outerRing.reserve(n);

    const auto& roofDesc = get_texture("roof");

    const U32 roofBase = U32(geom->verts.size());

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
        geom->verts.push_back(v);
    }

    // earcut returns indices into the ring (0-based); shift by roofBase.
    auto rawIdx = mapbox::earcut<U32>(polygon);

    // earcut produces CW winding in screen space (Y-down); UE uses Y-up
    // with CCW front-face convention.  Since the UE viewport has Y pointing
    // north (up in world), earcut's XY matches the UE XY plane and the
    // winding is already CCW when viewed from above (Z+).  Reverse each
    // triangle to match the expected outward (Z+) normal.
    for (U32 t = 0; t + 2 < U32(rawIdx.size()); t += 3) {
        geom->indices.push_back(roofBase + rawIdx[t + 0]);
        geom->indices.push_back(roofBase + rawIdx[t + 2]); // swap t+1/t+2
        geom->indices.push_back(roofBase + rawIdx[t + 1]);
    }
};

} // namespace nv