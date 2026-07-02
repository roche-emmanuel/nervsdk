#include <nvk/pcg/CellTextureAtlas.h>

namespace nv {

void to_json(Json& j, const CellTextureEntry& e) {
    j = {
        {"id", e.id},
        {"file", e.file},
        {"size", Json::array({e.xsize, e.ysize})},
    };
}
void from_json(const Json& j, CellTextureEntry& e) {
    j.at("file").get_to(e.file);

    if (j.contains("id")) {
        j.at("id").get_to(e.id);
    } else {
        // Use the filename as id:
        e.id = get_filename(e.file, false);
    }

    get_opt(j, "id", e.id);
    if (j.contains("size") && j.at("size").is_array() &&
        j.at("size").size() >= 2) {
        e.xsize = j.at("size")[0].get<I32>();
        e.ysize = j.at("size")[1].get<I32>();
    }
}

void to_json(Json& j, const CellTextureAtlasDesc& c) {
    j = {
        {"slot_size", c.slotSize},
        {"grid_xsize", c.gridXSize},
        {"grid_ysize", c.gridYSize},
    };
    Json arr = Json::array();
    for (const CellTextureEntry& e : c.content)
        arr.push_back(e);
    j["content"] = arr;
}
void from_json(const Json& j, CellTextureAtlasDesc& c) {
    get_opt(j, "slot_size", c.slotSize);
    get_opt(j, "grid_xsize", c.gridXSize);
    get_opt(j, "grid_ysize", c.gridYSize);
    if (j.contains("content")) {
        c.content.clear();
        for (const auto& item : j.at("content"))
            c.content.emplace_back(item.get<CellTextureEntry>());
    }
}

void CellTextureAtlasLayout::build(I32 slotSize, I32 gridXSize, I32 gridYSize,
                                   const Vector<CellTextureEntry>& content,
                                   const String& dataDir) {
    _slotSize = std::max(slotSize, 1);
    _gridXSize = std::max(gridXSize, 1);
    _gridYSize = std::max(gridYSize, 1);
    _layerWidthPx = _slotSize * _gridXSize;
    _layerHeightPx = _slotSize * _gridYSize;
    _numLayers = 0;
    _occupancy.clear();
    _descById.clear();

    for (const auto& entry : content) {
        place_entry(entry, dataDir);
    }

    logINFO("CellTextureAtlasLayout: {} textures packed into {} layer(s) "
            "({}x{} slots of {}px -> {}x{}px per layer).",
            _descById.size(), _numLayers, _gridXSize, _gridYSize, _slotSize,
            _layerWidthPx, _layerHeightPx);
}

auto CellTextureAtlasLayout::get_cell_texture_desc(const String& id) const
    -> const CellTextureDesc& {
    auto it = _descById.find(id);
    if (it == _descById.end()) {
        logERROR("CellTextureAtlasLayout: unknown texture id '{}'.", id);
        return _invalidDesc;
    }
    return it->second;
}

auto CellTextureAtlasLayout::find_free_slot(U32 layer, I32 xsize, I32 ysize,
                                            Vec2i& outSlot) const -> bool {
    const auto& occ = _occupancy[layer];
    for (I32 sy = 0; sy + ysize <= _gridYSize; ++sy) {
        for (I32 sx = 0; sx + xsize <= _gridXSize; ++sx) {
            bool free = true;
            for (I32 dy = 0; dy < ysize && free; ++dy) {
                for (I32 dx = 0; dx < xsize; ++dx) {
                    if (occ[size_t(sy + dy) * _gridXSize + size_t(sx + dx)]) {
                        free = false;
                        break;
                    }
                }
            }
            if (free) {
                outSlot = {sx, sy};
                return true;
            }
        }
    }
    return false;
}

void CellTextureAtlasLayout::mark_occupied(U32 layer, const Vec2i& slot,
                                           I32 xsize, I32 ysize) {
    auto& occ = _occupancy[layer];
    for (I32 dy = 0; dy < ysize; ++dy) {
        for (I32 dx = 0; dx < xsize; ++dx) {
            occ[size_t(slot.y() + dy) * _gridXSize + size_t(slot.x() + dx)] =
                true;
        }
    }
}

auto CellTextureAtlasLayout::compute_uv(const Vec2i& slot, I32 xsize,
                                        I32 ysize) const -> Box2d {
    // Inset half a texel on every border so bilinear/mip sampling never
    // bleeds into a neighbouring slot.
    const F64 x0 = F64(slot.x() * _slotSize) + 0.5;
    const F64 x1 = F64((slot.x() + xsize) * _slotSize) - 0.5;
    const F64 y0 = F64(slot.y() * _slotSize) + 0.5;
    const F64 y1 = F64((slot.y() + ysize) * _slotSize) - 0.5;

    return Box2d(x0 / F64(_layerWidthPx), x1 / F64(_layerWidthPx),
                 y0 / F64(_layerHeightPx), y1 / F64(_layerHeightPx));
}

auto CellTextureAtlasLayout::resolve_footprint(const CellTextureEntry& entry,
                                               const String& dataDir) const
    -> Vec2i {
    if (entry.xsize > 0 && entry.ysize > 0) {
        return {entry.xsize, entry.ysize};
    }

    NVCHK(!entry.file.empty(),
          "CellTextureAtlasLayout: '{}' has no explicit size and no file "
          "to auto-detect it from.",
          entry.id);

    const String path = nv::is_absolute_path(entry.file)
                            ? entry.file
                            : nv::get_path(dataDir, entry.file);

    NVCHK(system_file_exists(path), "Missing image file: {}", path);

    const Vec2i pxSize = get_image_size(path);
    if (pxSize.x() <= 0 || pxSize.y() <= 0) {
        logWARN("CellTextureAtlasLayout: '{}' auto-detect failed for "
                "'{}' — defaulting to 1x1 slot.",
                entry.id, path);
        return {1, 1};
    }

    const I32 xslots = std::max(1, (pxSize.x() + _slotSize - 1) / _slotSize);
    const I32 yslots = std::max(1, (pxSize.y() + _slotSize - 1) / _slotSize);

    logDEBUG("CellTextureAtlasLayout: '{}' auto-detected {}x{}px -> "
             "{}x{} slots.",
             entry.id, pxSize.x(), pxSize.y(), xslots, yslots);

    return {xslots, yslots};
}

void CellTextureAtlasLayout::place_entry(const CellTextureEntry& entry,
                                         const String& dataDir) {
    const Vec2i footprint = resolve_footprint(entry, dataDir);
    const I32 xsize = std::max(footprint.x(), 1);
    const I32 ysize = std::max(footprint.y(), 1);

    if (xsize > _gridXSize || ysize > _gridYSize) {
        logERROR("CellTextureAtlasLayout: '{}' is {}x{} slots, larger than "
                 "the layer grid {}x{}.",
                 entry.id, xsize, ysize, _gridXSize, _gridYSize);
        NVCHK(false, "Texture larger than layer grid.");
    }

    U32 layer = 0;
    Vec2i slot{0, 0};
    for (;; ++layer) {
        if (layer >= _numLayers) {
            _occupancy.emplace_back(
                Vector<bool>(size_t(_gridXSize) * size_t(_gridYSize), false));
            _numLayers = layer + 1;
        }
        if (find_free_slot(layer, xsize, ysize, slot))
            break;
    }

    mark_occupied(layer, slot, xsize, ysize);

    CellTextureDesc desc;
    desc.valid = true;
    desc.layer = layer;
    desc.index = _descById.size();
    desc.slot = slot;
    desc.sizeInSlots = {xsize, ysize};
    desc.uv = compute_uv(slot, xsize, ysize);

    logDEBUG("CellTextureAtlasLayout: id='{}' -> layer {} slot [{},{}] "
             "size [{}x{}] uv {}.",
             entry.id, layer, slot.x(), slot.y(), xsize, ysize, desc.uv);

    _descById.emplace(entry.id, desc);
}

void remap_uv_to_atlas(F32 rawU, F32 rawV, const CellTextureDesc& desc,
                       F32& outU, F32& outV) {
    const F32 fu = rawU - std::floor(rawU);
    const F32 fv = rawV - std::floor(rawV);
    outU = F32(desc.uv.xmin) + fu * F32(desc.uv.width());
    outV = F32(desc.uv.ymin) + fv * F32(desc.uv.height());
}
CellTextureAtlasLayout::CellTextureAtlasLayout(const CellTextureAtlasDesc& desc,
                                               const String& dataDir) {
    build(desc.slotSize, desc.gridXSize, desc.gridYSize, desc.content, dataDir);
}
} // namespace nv