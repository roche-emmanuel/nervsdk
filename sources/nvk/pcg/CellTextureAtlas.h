#ifndef _NV_CELLTEXTUREATLAS_H_
#define _NV_CELLTEXTUREATLAS_H_

#include <nvk/math/Box2.h>
#include <nvk_common.h>

namespace nv {

struct CellTextureEntry {
    String id;
    String file;
    I32 xsize = 1;
    I32 ysize = 1;
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

    I32 _slotSize{512};
    I32 _gridXSize{1};
    I32 _gridYSize{1};
    I32 _layerWidthPx{0};
    I32 _layerHeightPx{0};
    U32 _numLayers{0};

    Vector<Vector<bool>> _occupancy; // one bitmap per layer
    UnorderedMap<String, CellTextureDesc> _descById;
    CellTextureDesc _invalidDesc;
};

// Wraps a repeating [0, N) UV coordinate and remaps it into desc.uv.
// Use everywhere a vertex UV0 is currently written against the raw texture
// — replaces the "whole layer is my texture" assumption.
void remap_uv_to_atlas(F32 rawU, F32 rawV, const CellTextureDesc& desc,
                       F32& outU, F32& outV);

} // namespace nv

#endif