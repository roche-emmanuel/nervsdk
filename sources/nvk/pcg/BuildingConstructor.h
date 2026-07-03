#ifndef _NV_BUILDINGCONSTRUCTOR_H_
#define _NV_BUILDINGCONSTRUCTOR_H_

#include <nvk/pcg/CellTextureAtlas.h>

namespace nv {

struct BuildingConstructor {
    U64 id;
    String subtype;
    String style;
    UnorderedMap<String, const CellTextureDesc*> textures;
    F64 uvScale{1.0};
    F64 bottomHeight{0.0};
    F64 topHeight{0.0};
    F64 buriedHeight{0.0};
    F64 levelHeight{350.0};
    U32 numPlacedDoors{0};
    TileGeom* geom;
    Vec2d origin;

    // ── Facade cursor state (set by start_facade, advanced by emit_/move_) ──
    // The cursor lives in facade-local coords: an along-wall offset in metres
    // and a base elevation in cm. World XY and tiling UV are derived from it,
    // so there is a single source of truth (no currentBL/currentUV to desync).
    Vec2d facadeOrigin;   // facade start corner `a` (world cm)
    Vec2d facadeDir;      // unit along-wall direction (XY)
    Vec2d facadeNormal;   // unit outward normal (XY)
    F64 facadeBaseZ{0.0}; // z reference for the continuous tiling v-origin (cm)
    F64 cursorU{0.0};     // along-wall cursor offset from origin (metres)
    F64 cursorZ{0.0};     // cursor base elevation (world cm, absolute)

    // ── Per-facade metrics, resolved once in create_facade ──────────────────
    const CellTextureDesc* wallDesc{nullptr};
    const CellTextureDesc* doorDesc{nullptr};
    const CellTextureDesc* windowDesc{nullptr};
    F64 facadeRunM{0.0}; // along-wall run length of the current edge (m)
    F64 curFloorHeightM{
        0.0}; // height of the floor currently being laid out (m)
    F64 winWidthM{0.0};
    F64 winHeightM{0.0};
    F64 doorWidthM{0.0};
    F64 doorHeightM{0.0};
    F64 sillM{1.0};         // window sill height above the floor line (m)
    F64 windowInsetM{0.15}; // recess depth for windows; 0 => flat (m)

    const CellTextureAtlasLayout* atlasLayout{nullptr};

    BuildingConstructor(U64 bid, String stype,
                        const CellTextureAtlasLayout& atlas, TileGeom* out);

    auto get_texture(const String& tname) -> const CellTextureDesc&;

    auto create_facade(const Vec2d& a, const Vec2d& b) -> bool;

    void create_roof(const Vector<Vec2d>& ring);

  private:
    // ── Unit helpers. uvScale converts cm -> metres (~0.01). ────────────────
    [[nodiscard]] auto m_to_cm(F64 m) const -> F64 { return m / uvScale; }
    [[nodiscard]] auto cm_to_m(F64 cm) const -> F64 { return cm * uvScale; }

    // World XY (cm) at the current along-wall cursor offset.
    [[nodiscard]] auto cursor_xy() const -> Vec2d {
        return facadeOrigin + facadeDir * m_to_cm(cursorU);
    }

    void start_facade(const Vec2d& a, const Vec2d& dir, F64 baseHeight);

    // ── Cursor navigation ───────────────────────────────────────────────────
    // move_by: du is along-wall metres, dz is vertical metres.
    void move_by(F64 duM, F64 dzM);
    // move_to: u is along-wall metres from the facade start; zCm is world cm.
    void move_to(F64 uM, F64 zCm);

    // ── Facade emitters (all sizes in metres; cursor advances by width) ─────
    // Core: draw one quad at the cursor and advance cursorU by widthM.
    //   tile==true  -> continuous wall tiling (uv0 = running metres)
    //   tile==false -> map one 0..1 element tile (uv0 = {0,0}, sized to dimsM)
    void emit_facade_quad(F64 widthM, F64 heightM, F64 zOffsetM,
                          const CellTextureDesc& desc, bool tile);

    // Wall filler. heightM < 0 => full current-floor height.
    void emit_facade_filler(F64 lengthM, F64 heightM = -1.0,
                            F64 zOffsetM = 0.0);

    // Single element (door/window). widthM/heightM < 0 => desc.dimsM.
    // insetM > 0 recesses the element and emits the 4 reveal quads.
    void emit_facade_element(const CellTextureDesc& desc, F64 zOffsetM = 0.0,
                             F64 insetM = 0.0, F64 widthM = -1.0,
                             F64 heightM = -1.0);

    void emit_facade_window();
    void emit_facade_door();

    // Lay out one floor: leading pier, [element, trailing pier]* .
    void build_floor(F64 floorBaseZcm, I32 nDoorsF, I32 nWindowsF,
                     I32 doorSlotF);

    // ── Low-level geometry ──────────────────────────────────────────────────
    void push_vertex(const Vec2d& p, F64 z, const Vec3d& n, const Vec2d& uv,
                     const CellTextureDesc& tdesc);
    void push_vertex(const Vec3d& p, const Vec3d& n, const Vec2d& uv,
                     const CellTextureDesc& tdesc);
    void push_vquad(const Vec3d& bl, const Vec2d& xdir, F64 height,
                    const Vec2d& n, const Vec2d& uv0,
                    const CellTextureDesc& tdesc);
    // General quad from an arbitrary 3D basis (spans in cm, uv spans in m).
    void push_quad(const Vec3d& originCm, const Vec3d& xspanCm,
                   const Vec3d& yspanCm, const Vec3d& n, const Vec2d& uv0,
                   F64 uSpanM, F64 vSpanM, const CellTextureDesc& tdesc);
    void push_tri_indices(U32 i0, U32 i1, U32 i2) const;
};

} // namespace nv

#endif