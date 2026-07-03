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
    const CellTextureAtlasLayout* atlasLayout{nullptr};

    BuildingConstructor(U64 bid, String stype,
                        const CellTextureAtlasLayout& atlas, TileGeom* out);

    auto get_texture(const String& tname) -> const CellTextureDesc&;

    auto create_facade(const Vec2d& a, const Vec2d& b) -> bool;

    void create_roof(const Vector<Vec2d>& ring);

  private:
    void push_vertex(const Vec2d& p, F64 z, const Vec3d& n, const Vec2d& uv,
                     const CellTextureDesc& tdesc);
    void push_vquad(const Vec3d& bl, const Vec2d& xdir, F64 height,
                    const Vec2d& n, const Vec2d& uv0,
                    const CellTextureDesc& tdesc);
    void push_tri_indices(U32 i0, U32 i1, U32 i2) const;
};

} // namespace nv

#endif