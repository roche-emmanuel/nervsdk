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
    Vec2d origin;
    const CellTextureAtlasLayout* atlasLayout{nullptr};

    BuildingConstructor(U64 bid, String stype,
                        const CellTextureAtlasLayout& atlas);

    auto get_texture(const String& tname) -> const CellTextureDesc&;

    auto create_facade(const Vec2d& a, const Vec2d& b,
                       Vector<CellVertex>& vertices, Vector<U32>& indices)
        -> bool;

    void create_roof(const Vector<Vec2d>& ring, Vector<CellVertex>& vertices,
                     Vector<U32>& indices);
};

} // namespace nv

#endif