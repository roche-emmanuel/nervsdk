#ifndef _NV_CELLTEXTUREATLAS_H_
#define _NV_CELLTEXTUREATLAS_H_

#include <nvk/math/Box2.h>
#include <nvk/pcg/overture_maps.h>
#include <nvk_common.h>

namespace nv {

struct CellTextureEntry {
    String id;
    String file;
    I32 xsize = -1;
    I32 ysize = -1;
    String type;
    Set<String> subtypes;
    String category;
    Set<String> styles;
    Vec2f dimsM;
};

void to_json(Json& j, const CellTextureEntry& e);

void from_json(const Json& j, CellTextureEntry& e);

struct CellTextureAtlasDesc {
    // Per-slot texel size; each layer is slotSize*gridXSize ×
    // slotSize*gridYSize.
    I32 slotSize = 512;
    I32 gridXSize = 1;
    I32 gridYSize = 1;
    Vector<CellTextureEntry> content;
    U32 seed = 1234;
};

void to_json(Json& j, const CellTextureAtlasDesc& c);

void from_json(const Json& j, CellTextureAtlasDesc& c);

// Resolved placement of one texture id inside the shared cells texture
// array. Computed deterministically from the ordered cells_material.content
// list, so PCGen (mesh UV baking) and ArgusWorldBuilder (UTexture2DArray
// construction) always agree as long as they build from the same config.
struct CellTextureDesc {
    bool valid{false};
    U32 layer{0}; // texture-array slice == vertex tex_idx
    U32 index{0};
    Vec2i slot{0, 0}; // slot coords within the layer
    Vec2i sizeInSlots{1, 1};
    Box2d uv; // half-texel-inset uv rect for one tile
    String type;
    Set<String> subtypes;
    String category;
    Set<String> styles;
    Vec2f dimsM;

    void scale_uv(F32 u, F32 v, F32& scaledU, F32& scaledV) const;
};

class CellTextureAtlasLayout {
  public:
    CellTextureAtlasLayout() = default;
    // dataDir resolves relative `file` paths for entries that need
    // auto-detected sizes (xsize<=0 or ysize<=0).
    explicit CellTextureAtlasLayout(const CellTextureAtlasDesc& desc,
                                    const String& dataDir);

    [[nodiscard]] auto get_cell_texture_desc(const String& id) const
        -> const CellTextureDesc&;

    void build(I32 slotSize, I32 gridXSize, I32 gridYSize,
               const Vector<CellTextureEntry>& content,
               const String& dataDir = "");

    [[nodiscard]] auto layer_width_px() const -> I32 { return _layerWidthPx; }
    [[nodiscard]] auto layer_height_px() const -> I32 { return _layerHeightPx; }
    [[nodiscard]] auto num_layers() const -> U32 { return _numLayers; }

    auto pick_style(const String& type, String& subtype, U64 elemId) const
        -> const String&;

    [[nodiscard]] auto
    pick_texture_desc(const String& type, const String& subtype,
                      const String& style, const String& category,
                      U64 elemId) const -> const CellTextureDesc&;

  private:
    void place_entry(const CellTextureEntry& entry, const String& dataDir);
    // Returns the entry footprint in slot units. Uses xsize/ysize directly
    // when both are set (>0); otherwise probes the image file's pixel size
    // (stb_image header read, no full decode) and rounds up to whole slots.
    [[nodiscard]] auto resolve_footprint(const CellTextureEntry& entry,
                                         const String& dataDir) const -> Vec2i;

    [[nodiscard]] auto find_free_slot(U32 layer, I32 xsize, I32 ysize,
                                      Vec2i& outSlot) const -> bool;
    void mark_occupied(U32 layer, const Vec2i& slot, I32 xsize, I32 ysize);
    [[nodiscard]] auto compute_uv(const Vec2i& slot, I32 xsize, I32 ysize) const
        -> Box2d;

    void generate_style_map();

    void generate_category_map();

    I32 _slotSize{512};
    I32 _gridXSize{1};
    I32 _gridYSize{1};
    I32 _layerWidthPx{0};
    I32 _layerHeightPx{0};
    U32 _numLayers{0};
    U32 _seed{1234};

    Vector<Vector<bool>> _occupancy; // one bitmap per layer
    UnorderedMap<String, CellTextureDesc> _descById;
    CellTextureDesc _invalidDesc;

    // Style map:
    UnorderedMap<String, Vector<String>> _stylesMap;

    // Category maps
    UnorderedMap<String, Vector<String>> _categoryMap;
};

// Wraps a repeating [0, N) UV coordinate and remaps it into desc.uv.
// Use everywhere a vertex UV0 is currently written against the raw texture
// — replaces the "whole layer is my texture" assumption.
void remap_uv_to_atlas(F32 rawU, F32 rawV, const CellTextureDesc& desc,
                       F32& outU, F32& outV);

struct BuildingConstructionContext {
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

    BuildingConstructionContext(U64 bid, String stype,
                                const CellTextureAtlasLayout& atlas);

    auto get_texture(const String& tname) -> const CellTextureDesc&;

    auto create_building_facade(const Vec2d& a, const Vec2d& b,
                                Vector<CellVertex>& vertices,
                                Vector<U32>& indices) -> bool;
};

} // namespace nv

#endif