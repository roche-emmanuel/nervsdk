#include <external/earcut.hpp>
#include <nvk/pcg/CellTextureAtlas.h>

namespace nv {

void to_json(Json& j, const CellTextureEntry& e) {
    j = {
        {"id", e.id},
        {"file", e.file},
        {"size", Json::array({e.xsize, e.ysize})},
        {"type", e.type},
        {"category", e.category},
        {"dimsM", e.dimsM},
    };
    if (!e.subtypes.empty()) {
        j["subtypes"] = e.subtypes;
    }
    if (!e.styles.empty()) {
        j["styles"] = e.styles;
    }
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
    j.at("type").get_to(e.type);
    j.at("category").get_to(e.category);
    j.at("dimsM").get_to(e.dimsM);
    get_opt(j, "subtypes", e.subtypes);
    get_opt(j, "styles", e.styles);

    if (e.subtypes.empty()) {
        e.subtypes.insert("default");
    }
    if (e.styles.empty()) {
        e.styles.insert("default");
    }

    NVCHK(e.dimsM.x() > 0.0 && e.dimsM.y() > 0.0,
          "Invalid dimensions in CettTextureEntry.");
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

    generate_style_map();
    generate_category_map();
}

auto CellTextureAtlasLayout::get_cell_texture_desc(const String& id) const
    -> const CellTextureDesc& {
    auto it = _descById.find(id);
    NVCHK(it != _descById.end(),
          "CellTextureAtlasLayout: unknown texture id '{}'.", id);
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
        return {std::max(1, (entry.xsize + _slotSize - 1) / _slotSize),
                std::max(1, (entry.ysize + _slotSize - 1) / _slotSize)};
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
        THROW_MSG("CellTextureAtlasLayout: '{}' is {}x{} slots, larger than "
                  "the layer grid {}x{}.",
                  entry.id, xsize, ysize, _gridXSize, _gridYSize);
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
    desc.type = entry.type;
    desc.subtypes = entry.subtypes;
    desc.category = entry.category;
    desc.styles = entry.styles;
    desc.dimsM = entry.dimsM;

    desc.sizeInSlots = {xsize, ysize};
    desc.uv = compute_uv(slot, xsize, ysize);

    logDEBUG("CellTextureAtlasLayout: id='{}' -> layer {} slot [{},{}] "
             "size [{}x{}] uv {}.",
             entry.id, layer, slot.x(), slot.y(), xsize, ysize, desc.uv);

    auto res = _descById.insert(std::make_pair(entry.id, desc));
    NVCHK(res.second, "Could not insert CellTextureDesc: duplicated id: {}",
          entry.id);
}

void remap_uv_to_atlas(F32 rawU, F32 rawV, const CellTextureDesc& desc,
                       F32& outU, F32& outV) {
    const F32 fu = rawU - std::floor(rawU);
    const F32 fv = rawV - std::floor(rawV);
    outU = F32(desc.uv.xmin) + fu * F32(desc.uv.width());
    outV = F32(desc.uv.ymin) + fv * F32(desc.uv.height());
}
CellTextureAtlasLayout::CellTextureAtlasLayout(const CellTextureAtlasDesc& desc,
                                               const String& dataDir)
    : _seed(desc.seed) {
    build(desc.slotSize, desc.gridXSize, desc.gridYSize, desc.content, dataDir);
}
void CellTextureAtlasLayout::generate_style_map() {
    // Generate the style map for all the type/subtype pairs:

    _stylesMap.clear();
    UnorderedMap<String, Set<String>> tempMap;

    for (const auto& it : _descById) {
        String prefix = it.second.type + "_";
        for (const auto& stype : it.second.subtypes) {
            auto key = prefix + stype;
            auto it2 = tempMap.find(key);
            if (it2 == tempMap.end()) {
                // Insert the set of styles as starting point:
                tempMap.insert(std::make_pair(key, it.second.styles));
            } else {
                // Append the new styles:
                it2->second.insert(it.second.styles.begin(),
                                   it.second.styles.end());
            }
        }
    }
    for (const auto& it : tempMap) {
        _stylesMap.insert(std::make_pair(
            it.first, Vector<String>{it.second.begin(), it.second.end()}));
    }

    logDEBUG("Generated style map:");
    for (const auto& it : _stylesMap) {
        logDEBUG("  - {}: {}", it.first, it.second);
    }
};

auto CellTextureAtlasLayout::pick_style(const String& type, String& subtype,
                                        U64 elemId) const -> const String& {
    String key = type + "_" + subtype;
    auto it = _stylesMap.find(key);
    if (it == _stylesMap.end()) {
        subtype = "default";
        key = type + "_" + subtype;
        it = _stylesMap.find(key);
    }
    NVCHK(it != _stylesMap.end(), "No style available for {}", key);

    // Once we have the style map we should select a style for this element.
    // Note: eventually we could assign weights/probability to each style,
    // And then generate an U32 value in range [0, 100000] for instance and
    // partition range range proportionally between all available style weights.
    // But for now, a simple uniform selection will do:

    U32 hash = hash_id_with_seed(elemId, _seed);
    U32 idx = hash % it->second.size();
    return it->second[idx];
};

void CellTextureAtlasLayout::generate_category_map() {
    _categoryMap.clear();

    for (const auto& it : _descById) {
        String prefix = it.second.type + "_";
        for (const auto& stype : it.second.subtypes) {
            for (const auto& style : it.second.styles) {

                auto key =
                    prefix + stype + "_" + style + "_" + it.second.category;

                auto it2 = _categoryMap.find(key);
                if (it2 == _categoryMap.end()) {
                    // Insert the set of styles as starting point:
                    _categoryMap.insert(
                        std::make_pair(key, Vector<String>{it.first}));
                } else {
                    // Append the new styles:
                    it2->second.emplace_back(it.first);
                }
            }
        }
    }

    logDEBUG("Generated category maps:");
    for (const auto& it : _categoryMap) {
        logDEBUG("  - {}: {}", it.first, it.second);
    }
};

auto CellTextureAtlasLayout::pick_texture_desc(
    const String& type, const String& subtype, const String& style,
    const String& category, U64 elemId) const -> const CellTextureDesc& {

    auto key = format_msg("{}_{}_{}_{}", type, subtype, style, category);

    auto it = _categoryMap.find(key);
    NVCHK(it != _categoryMap.end(), "No entry for texture category {}", key);

    StringID catId = SID(category.c_str());
    U32 hash = hash_id_with_seed(elemId + catId, _seed);
    U32 idx = hash % it->second.size();
    const auto& descId = it->second[idx];
    return get_cell_texture_desc(descId);
};

void CellTextureDesc::scale_uv(F32 u, F32 v, F32& scaledU, F32& scaledV) const {
    scaledU = u / dimsM.x();
    scaledV = v / dimsM.y();
}

} // namespace nv