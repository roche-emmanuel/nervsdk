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
    // Per-entry gutter width in pixels on each side of the content rect.
    // < 0 inherits CellTextureAtlasDesc::padSize.
    I32 pad = -1;
    // Gutter fill mode, i.e. the address mode this texture is *meant* to be
    // sampled with:
    //   true  -> wrap: the gutter holds copies of the opposite content
    //            borders. Correct for anything sampled with a repeating UV
    //            (wall fillers, road surfaces) — a filter tap crossing the
    //            content edge then reads what TA_Wrap would have returned on
    //            a standalone texture.
    //   false -> clamp: the gutter replicates the nearest content edge.
    //            Correct for an element mapped exactly once over 0..1
    //            (windows, doors), where wrapping would fold the far edge in.
    bool tiling = true;
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
    // Default gutter width in pixels, taken *out of* every entry's slot
    // footprint: a 2x4 entry at slotSize 64 owns 128x256px and gets a
    // 120x248px content rect centred in it. Keep it a multiple of 4 so the
    // content rect stays aligned to BC block boundaries.
    I32 padSize = 4;
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
    Vec2i slot{0, 0};        // slot coords within the layer
    Vec2i sizeInSlots{1, 1}; // footprint in slots (content + gutter)
    I32 pad{0};              // gutter width in px on each side
    bool tiling{true};       // gutter fill mode (see CellTextureEntry)
    Vec2i originPx{0, 0};    // content rect min corner, in layer pixels
    Vec2i sizePx{0, 0};      // content rect size, in pixels
    // UV rect of the *content* rect, aligned to its pixel edges (not texel
    // centres): a repeating uv of 0 lands on the left edge of the first
    // content texel and 1 on the right edge of the last, so the tiling
    // period is exact and the filter spills into the gutter, which holds the
    // right values. No half-texel inset — see compute_uv().
    Box2d uv;
    String type;
    Set<String> subtypes;
    String category;
    Set<String> styles;
    Vec2f dimsM;

    void scale_uv(F32 u, F32 v, F32& scaledU, F32& scaledV) const;

    // Full slot footprint (content rect inflated by the gutter), in layer
    // pixels. The blitter needs this; everything sampling-related wants the
    // content rect instead.
    [[nodiscard]] auto footprint_origin_px() const -> Vec2i {
        return {originPx.x() - pad, originPx.y() - pad};
    }
    [[nodiscard]] auto footprint_size_px() const -> Vec2i {
        return {sizePx.x() + 2 * pad, sizePx.y() + 2 * pad};
    }
};

class CellTextureAtlasLayout {
  public:
    CellTextureAtlasLayout() = default;
    // dataDir resolves relative `file` paths for entries that need
    // auto-detected sizes (xsize<=0 or ysize<=0).
    explicit CellTextureAtlasLayout(const CellTextureAtlasDesc& desc,
                                    const String& dataDir);

    [[nodiscard]] auto find_cell_texture_desc(const String& id) const
        -> const CellTextureDesc*;
    [[nodiscard]] auto get_cell_texture_desc(const String& id) const
        -> const CellTextureDesc&;

    void build(I32 slotSize, I32 gridXSize, I32 gridYSize,
               const Vector<CellTextureEntry>& content,
               const String& dataDir = "", I32 padSize = 0);

    [[nodiscard]] auto layer_width_px() const -> I32 { return _layerWidthPx; }
    [[nodiscard]] auto layer_height_px() const -> I32 { return _layerHeightPx; }
    [[nodiscard]] auto num_layers() const -> U32 { return _numLayers; }
    [[nodiscard]] auto slot_size() const -> I32 { return _slotSize; }
    [[nodiscard]] auto pad_size() const -> I32 { return _padSize; }

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
    // Note the requested size is the *footprint*, not the content rect: the
    // gutter is carved out of it, so the usable content is slightly smaller.
    [[nodiscard]] auto resolve_footprint(const CellTextureEntry& entry,
                                         const String& dataDir) const -> Vec2i;

    // Per-entry gutter width, falling back to the atlas-wide default.
    [[nodiscard]] auto resolve_pad(const CellTextureEntry& entry) const -> I32;

    [[nodiscard]] auto find_free_slot(U32 layer, I32 xsize, I32 ysize,
                                      Vec2i& outSlot) const -> bool;
    void mark_occupied(U32 layer, const Vec2i& slot, I32 xsize, I32 ysize);
    [[nodiscard]] auto compute_uv(const Vec2i& originPx,
                                  const Vec2i& sizePx) const -> Box2d;

    void generate_style_map();

    void generate_category_map();

    I32 _slotSize{512};
    I32 _gridXSize{1};
    I32 _gridYSize{1};
    I32 _padSize{4};
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

} // namespace nv

#endif