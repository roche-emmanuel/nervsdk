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

void BuildingConstructor::push_vertex(const Vec2d& p, F64 z, const Vec3d& n,
                                      const Vec2d& uv,
                                      const CellTextureDesc& tdesc) {
    CellVertex v{};
    v.px = F32(p.x() - origin.x());
    v.py = F32(p.y() - origin.y());
    v.pz = F32(z);
    v.nx = F32(n.x());
    v.ny = F32(n.y());
    v.nz = F32(n.z());
    tdesc.scale_uv(F32(uv.x()), F32(uv.y()), v.u0, v.v0);
    v.texIdx = F32(tdesc.index);
    geom->verts.push_back(v);
};

// 3D-position overload used by push_quad for reveals that leave the wall plane.
void BuildingConstructor::push_vertex(const Vec3d& p, const Vec3d& n,
                                      const Vec2d& uv,
                                      const CellTextureDesc& tdesc) {
    push_vertex(p.xy(), p.z(), n, uv, tdesc);
}

void BuildingConstructor::push_tri_indices(U32 i0, U32 i1, U32 i2) const {
    geom->indices.push_back(i0);
    geom->indices.push_back(i1);
    geom->indices.push_back(i2);
}

// bl = bottom-left corner (world cm, z absolute); xdir = along-wall vector
// (cm); height = cm; uv0 = tiling UV origin in *metres* (scale_uv divides
// by dimsM). An element passes uv0={0,0} and sizes the quad to dimsM so it
// maps exactly one 0..1 tile; a filler passes the running metre offset so
// the wall texture tiles continuously.
void BuildingConstructor::push_vquad(const Vec3d& bl, const Vec2d& xdir,
                                     F64 height, const Vec2d& n,
                                     const Vec2d& uv0,
                                     const CellTextureDesc& tdesc) {
    const Vec2d p0 = bl.xy();
    const Vec2d p1 = p0 + xdir;
    const F64 len = xdir.length();
    const U32 base = U32(geom->verts.size());
    Vec3d norm{n, 0};

    push_vertex(p0, bl.z(), norm, uv0, tdesc); // bottom-left
    push_vertex(p0, bl.z() + height, norm,     // top-left
                uv0 + Vec2d(0, height * uvScale), tdesc);
    push_vertex(p1, bl.z(), norm, uv0 + Vec2d(len * uvScale, 0),
                // bottom-right
                tdesc);
    push_vertex(p1, bl.z() + height, norm,
                uv0 + Vec2d(len * uvScale, height * uvScale), // top-right
                tdesc);

    // CCW winding viewed from outside (BL, TL, BR, TR = base 0,1,2,3):
    push_tri_indices(base + 0, base + 2, base + 1);
    push_tri_indices(base + 1, base + 2, base + 3);
};

// General quad from an arbitrary 3D basis. origin is the BL corner (world cm);
// xspan/yspan are the edge vectors in cm; uSpanM/vSpanM are the UV extents in
// metres. Winding matches push_vquad, so xspan x yspan must point along n for
// the geometric front face to agree with the shading normal.
void BuildingConstructor::push_quad(const Vec3d& originCm, const Vec3d& xspanCm,
                                    const Vec3d& yspanCm, const Vec3d& n,
                                    const Vec2d& uv0, F64 uSpanM, F64 vSpanM,
                                    const CellTextureDesc& tdesc) {
    const U32 base = U32(geom->verts.size());

    push_vertex(originCm, n, uv0, tdesc); // BL
    push_vertex(originCm + yspanCm, n,    // TL
                uv0 + Vec2d(0, vSpanM), tdesc);
    push_vertex(originCm + xspanCm, n, // BR
                uv0 + Vec2d(uSpanM, 0), tdesc);
    push_vertex(originCm + xspanCm + yspanCm, n, // TR
                uv0 + Vec2d(uSpanM, vSpanM), tdesc);

    push_tri_indices(base + 0, base + 2, base + 1);
    push_tri_indices(base + 1, base + 2, base + 3);
}

void BuildingConstructor::start_facade(const Vec2d& a, const Vec2d& dir,
                                       F64 baseHeight) {
    facadeOrigin = a;
    facadeDir = dir.normalized();
    // Outward normal (CW 90 deg of the forward direction = right-hand side of
    // a CCW ring):
    facadeNormal = Vec2d(facadeDir.y(), -facadeDir.x());
    facadeBaseZ = baseHeight;
    cursorU = 0.0;
    cursorZ = baseHeight;
}

void BuildingConstructor::move_by(F64 duM, F64 dzM) {
    cursorU += duM;
    cursorZ += m_to_cm(dzM);
}

void BuildingConstructor::move_to(F64 uM, F64 zCm) {
    cursorU = uM;
    cursorZ = zCm;
}

// Core emitter: draw one quad at the cursor, then advance cursorU by widthM.
// Continuous tiling reads the running metre offset (cursorU) and the height
// above facadeBaseZ; element tiling maps a single 0..1 tile sized to dimsM.
void BuildingConstructor::emit_facade_quad(F64 widthM, F64 heightM,
                                           F64 zOffsetM,
                                           const CellTextureDesc& desc,
                                           bool tile) {
    const F64 zBottomCm = cursorZ + m_to_cm(zOffsetM);

    if (widthM > 1e-4 && heightM > 1e-4) {
        const Vec2d uv0 = tile
                              ? Vec2d(cursorU, cm_to_m(zBottomCm - facadeBaseZ))
                              : Vec2d(0.0, 0.0);
        push_vquad({cursor_xy(), zBottomCm}, facadeDir * m_to_cm(widthM),
                   m_to_cm(heightM), facadeNormal, uv0, desc);
    }

    cursorU += widthM; // always advance, even for a skipped degenerate quad
}

void BuildingConstructor::emit_facade_filler(F64 lengthM, F64 heightM,
                                             F64 zOffsetM) {
    const F64 h = (heightM < 0.0) ? curFloorHeightM : heightM;
    emit_facade_quad(lengthM, h, zOffsetM, *wallDesc, /*tile=*/true);
}

void BuildingConstructor::emit_facade_element(const CellTextureDesc& desc,
                                              F64 zOffsetM, F64 insetM,
                                              F64 widthM, F64 heightM) {
    const F64 w = (widthM < 0.0) ? F64(desc.dimsM.x()) : widthM;
    const F64 h = (heightM < 0.0) ? F64(desc.dimsM.y()) : heightM;

    if (insetM <= 1e-4) {
        emit_facade_quad(w, h, zOffsetM, desc, /*tile=*/false);
        return;
    }

    // ── Recessed element: draw the face pushed inward, then box the reveal ──
    const F64 zBottomCm = cursorZ + m_to_cm(zOffsetM);
    const F64 wCm = m_to_cm(w);
    const F64 hCm = m_to_cm(h);
    const F64 dCm = m_to_cm(insetM);

    const Vec3d eu{facadeDir, 0.0};       // along-wall
    const Vec3d en{facadeNormal, 0.0};    // outward
    const Vec3d ez{Vec2d(0.0, 0.0), 1.0}; // up

    const Vec3d outerBL{cursor_xy(), zBottomCm};
    const Vec3d innerBL = outerBL - en * dCm;

    // Recessed face (still faces outward):
    push_vquad(innerBL, facadeDir * wCm, hCm, facadeNormal, {0, 0}, desc);

    // Four reveals with the wall texture (spans ordered so front == normal):
    // left jamb faces +u; right jamb -u; sill +z; head -z.
    push_quad(outerBL, en * (-dCm), ez * hCm, eu, {0, 0}, insetM, h, *wallDesc);
    push_quad(outerBL + eu * wCm, ez * hCm, en * (-dCm), eu * (-1.0), {0, 0}, h,
              insetM, *wallDesc);
    push_quad(outerBL, eu * wCm, en * (-dCm), ez, {0, 0}, w, insetM, *wallDesc);
    push_quad(outerBL + ez * hCm, en * (-dCm), eu * wCm, ez * (-1.0), {0, 0},
              insetM, w, *wallDesc);

    cursorU += w;
}

// window column = [under-sill filler] [glass] [head filler], stacked at one U.
void BuildingConstructor::emit_facade_window() {
    const F64 headM = curFloorHeightM - (sillM + winHeightM);

    emit_facade_filler(winWidthM, sillM, 0.0); // under-sill band
    move_by(-winWidthM, 0.0);
    emit_facade_element(*windowDesc, sillM, windowInsetM); // glass at sill
    move_by(-winWidthM, 0.0);
    emit_facade_filler(winWidthM, std::max(0.0, headM),
                       sillM + winHeightM); // head band
}

// door column = [door on the floor] [head filler above].
void BuildingConstructor::emit_facade_door() {
    const F64 headM = curFloorHeightM - doorHeightM;

    emit_facade_element(*doorDesc, 0.0, 0.0); // door sits on the floor line
    move_by(-doorWidthM, 0.0);
    emit_facade_filler(doorWidthM, std::max(0.0, headM), doorHeightM);
}

void BuildingConstructor::build_floor(F64 floorBaseZcm, I32 nDoorsF,
                                      I32 nWindowsF, I32 doorSlotF) {
    const I32 nelems = nDoorsF + nWindowsF;
    const F64 usedM = nDoorsF * doorWidthM + nWindowsF * winWidthM;
    const F64 pierM =
        nelems > 0 ? (facadeRunM - usedM) / F64(nelems + 1) : facadeRunM;

    move_to(0.0, floorBaseZcm); // cursor to the start of this floor's run

    if (pierM < 0.0) { // openings don't fit — fall back to a plain wall
        emit_facade_filler(facadeRunM, curFloorHeightM, 0.0);
        return;
    }

    emit_facade_filler(pierM); // leading pier (full-floor height)

    for (I32 j = 0; j < nelems; ++j) {
        const bool isDoor = (nDoorsF > 0) && (j == doorSlotF);
        if (isDoor)
            emit_facade_door();
        else
            emit_facade_window();

        emit_facade_filler(pierM); // trailing pier
    }
}

auto BuildingConstructor::create_facade(const Vec2d& a, const Vec2d& b)
    -> bool {

    Vec2d edgeDir = b - a;
    const F64 edgeLen = edgeDir.length();
    if (edgeLen < 1.0)
        return false; // degenerate edge < 1 cm

    edgeDir = edgeDir / edgeLen;

    start_facade(a, edgeDir, bottomHeight);

    wallDesc = &get_texture("wall");
    doorDesc = &get_texture("door");
    windowDesc = &get_texture("window");

    const F64 edgeLenM = edgeLen * uvScale;
    facadeRunM = edgeLenM;

    // ── Skirt (below the finished-floor elevation): one plain wall band ─────
    const F64 skirtCm = std::max(0.0, buriedHeight - bottomHeight);
    if (skirtCm > 1.0) {
        move_to(0.0, bottomHeight);
        curFloorHeightM = cm_to_m(skirtCm);
        emit_facade_filler(edgeLenM); // full skirt band, continuous tiling
    }

    // ── Floor grid spans [buriedHeight (ffe) .. topHeight] ─────────────────
    const F64 archHeightCm = std::max(0.0, topHeight - buriedHeight);
    // levelHeight is expected in cm (same convention as the original).
    const U32 nfloors =
        std::max(1U, U32(std::llround(archHeightCm / levelHeight)));
    const F64 floorHeightCm = archHeightCm / F64(nfloors);
    const F64 floorHeightM = floorHeightCm * uvScale;

    doorWidthM = doorDesc->dimsM.x();
    doorHeightM = doorDesc->dimsM.y();
    winWidthM = windowDesc->dimsM.x();
    winHeightM = windowDesc->dimsM.y();
    sillM = 1.0;              // window sill height above the floor line
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

    // Ground floor carries the door; every upper floor reuses one pattern.
    F64 floorBaseZ = buriedHeight;
    for (U32 fi = 0; fi < nfloors; ++fi) {
        curFloorHeightM = floorHeightM;
        if (fi == 0)
            build_floor(floorBaseZ, nDoors, nWinGround, doorSlot);
        else
            build_floor(floorBaseZ, 0, nWinUpper, -1);
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