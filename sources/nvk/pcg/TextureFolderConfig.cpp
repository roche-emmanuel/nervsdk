#include <nvk/pcg/TextureFolderConfig.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <map>
#include <set>

namespace nv {

namespace {

struct MapToken {
    const char* token;
    TextureMapKind kind;
    I32 rank; // normals: GL 2, plain 1, DX 0
};

// Longest first: "nor_gl" must be tried before "nor".
const MapToken kTokens[] = {
    {"ambient_occlusion", TextureMapKind::ao, 0},
    {"ambientocclusion", TextureMapKind::ao, 0},
    {"normal_gl", TextureMapKind::normal, 2},
    {"normal_dx", TextureMapKind::normal, 0},
    {"displacement", TextureMapKind::disp, 0},
    {"translucency", TextureMapKind::ignored, 0},
    {"transmission", TextureMapKind::ignored, 0},
    {"thumbnail", TextureMapKind::ignored, 0},
    {"reflection", TextureMapKind::reflect, 0},
    {"base_color", TextureMapKind::diffuse, 0},
    {"glossiness", TextureMapKind::ignored, 0},
    {"basecolor", TextureMapKind::diffuse, 0},
    {"roughness", TextureMapKind::rough, 0},
    {"metalness", TextureMapKind::metal, 0},
    {"occlusion", TextureMapKind::ao, 0},
    {"specular", TextureMapKind::reflect, 0},
    {"emission", TextureMapKind::ignored, 0},
    {"emissive", TextureMapKind::ignored, 0},
    {"normalgl", TextureMapKind::normal, 2},
    {"normaldx", TextureMapKind::normal, 0},
    {"metallic", TextureMapKind::metal, 0},
    {"diffuse", TextureMapKind::diffuse, 0},
    {"opacity", TextureMapKind::ignored, 0},
    {"preview", TextureMapKind::ignored, 0},
    {"reflect", TextureMapKind::reflect, 0},
    {"sheen", TextureMapKind::ignored, 0},
    {"albedo", TextureMapKind::diffuse, 0},
    {"colour", TextureMapKind::diffuse, 0},
    {"normal", TextureMapKind::normal, 1},
    {"height", TextureMapKind::disp, 0},
    {"alpha", TextureMapKind::ignored, 0},
    {"color", TextureMapKind::diffuse, 0},
    {"gloss", TextureMapKind::ignored, 0},
    {"nor_gl", TextureMapKind::normal, 2},
    {"nor_dx", TextureMapKind::normal, 0},
    {"thumb", TextureMapKind::ignored, 0},
    {"metal", TextureMapKind::metal, 0},
    {"rough", TextureMapKind::rough, 0},
    {"spec", TextureMapKind::reflect, 0},
    {"refl", TextureMapKind::reflect, 0},
    {"mask", TextureMapKind::ignored, 0},
    {"bump", TextureMapKind::ignored, 0},
    {"diff", TextureMapKind::diffuse, 0},
    {"disp", TextureMapKind::disp, 0},
    {"norm", TextureMapKind::normal, 1},
    {"nrm", TextureMapKind::normal, 1},
    {"nor", TextureMapKind::normal, 1},
    {"arm", TextureMapKind::arm, 0},
    {"orm", TextureMapKind::arm, 0},
    {"col", TextureMapKind::diffuse, 0},
    {"occ", TextureMapKind::ao, 0},
    {"ao", TextureMapKind::ao, 0},
};

auto lower(String s) -> String {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return char(std::tolower(c)); });
    return s;
}

auto ends_with(const String& s, const String& suffix) -> bool {
    return s.size() >= suffix.size() &&
           s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// "_2k" / "_2K" / "_2K-PNG" at the end of `s`: removes it and returns the
// number of K, 0 when there is none. `allowFormat` accepts the "-PNG" tail.
auto strip_resolution(String& s, bool allowFormat) -> I32 {
    const size_t us = s.rfind('_');
    if (us == String::npos || us + 1 >= s.size()) {
        return 0;
    }
    const String seg = lower(s.substr(us + 1));
    size_t i = 0;
    while (i < seg.size() && std::isdigit(static_cast<unsigned char>(seg[i]))) {
        ++i;
    }
    if (i == 0 || i >= seg.size() || seg[i] != 'k') {
        return 0;
    }
    ++i;
    if (i < seg.size()) {
        if (!allowFormat || seg[i] != '-' || i + 1 >= seg.size()) {
            return 0;
        }
        for (size_t k = i + 1; k < seg.size(); ++k) {
            if (!std::isalnum(static_cast<unsigned char>(seg[k]))) {
                return 0;
            }
        }
    }
    const I32 res = std::atoi(seg.c_str());
    s.erase(us);
    return std::max(res, 1);
}

auto image_ext_rank(const String& ext) -> I32 {
    static const char* const kExts[] = {"png", "jpg", "jpeg", "tga", "bmp",
                                        "exr", "hdr", "tif",  "tiff", "webp"};
    for (I32 i = 0; i < I32(std::size(kExts)); ++i) {
        if (ext == kExts[i]) {
            return i;
        }
    }
    return -1;
}

auto split_ext(const String& fname, String& stem, String& ext) -> void {
    const size_t dot = fname.rfind('.');
    if (dot == String::npos) {
        stem = fname;
        ext.clear();
    } else {
        stem = fname.substr(0, dot);
        ext = lower(fname.substr(dot + 1));
    }
}

auto normalize_folder(String folder) -> String {
    std::replace(folder.begin(), folder.end(), '\\', '/');
    while (!folder.empty() && folder.back() == '/') {
        folder.pop_back();
    }
    while (folder.size() >= 2 && folder[0] == '.' && folder[1] == '/') {
        folder.erase(0, 2);
    }
    return folder;
}

auto parse_res_limit(const Json& v) -> I32 {
    if (v.is_number()) {
        return v.get<I32>();
    }
    if (v.is_string()) { // "2k"
        String tmp = "_" + lower(v.get<String>());
        return strip_resolution(tmp, false);
    }
    return 0;
}

struct Candidate {
    String rel; // path relative to the folder
    I32 res{0};
    I32 rank{0};
    I32 extRank{0};
};

// Is `a` a better pick than `b` for the same map kind?
auto better(const Candidate& a, const Candidate& b, I32 maxRes) -> bool {
    const bool aOver = maxRes > 0 && a.res > maxRes;
    const bool bOver = maxRes > 0 && b.res > maxRes;
    if (aOver != bOver) {
        return bOver; // the one within the limit wins
    }
    if (aOver) { // both above the limit: the closest to it
        if (a.res != b.res) {
            return a.res < b.res;
        }
    } else if (a.res != b.res) {
        return a.res > b.res;
    }
    if (a.rank != b.rank) {
        return a.rank > b.rank;
    }
    if (a.extRank != b.extRank) {
        return a.extRank < b.extRank;
    }
    return a.rel < b.rel;
}

struct Group {
    String subdir;
    String baseId;
    std::map<TextureMapKind, Candidate> maps;
    I32 unrecognizedRes{0};
};

const char* const kMapKeys[] = {"file", "normal", "arm", "ao", "rough", "metal", "reflect", "disp"};

auto key_of(TextureMapKind kind) -> const char* {
    return kMapKeys[size_t(kind)];
}

void merge_into(Json& dst, const Json& src, const std::set<String>& skip) {
    for (auto it = src.begin(); it != src.end(); ++it) {
        if (skip.count(it.key()) == 0) {
            dst[it.key()] = it.value();
        }
    }
}

} // namespace

auto parse_texture_file_name(const String& stem) -> TextureFileName {
    TextureFileName out;
    String s = stem;
    I32 res = strip_resolution(s, false);
    const String low = lower(s);

    for (const auto& tok : kTokens) {
        const String suffix = String("_") + tok.token;
        const String suffix2 = String("-") + tok.token;
        if (low.size() > suffix.size() &&
            (ends_with(low, suffix) || ends_with(low, suffix2))) {
            String base = s.substr(0, s.size() - suffix.size());
            // AmbientCG: "Name_2K-PNG_Color", the resolution sits before the map.
            const I32 res2 = strip_resolution(base, true);
            if (res == 0) {
                res = res2;
            }
            if (base.empty()) {
                break;
            }
            out.baseId = base;
            out.kind = tok.kind;
            out.recognized = true;
            out.resolutionK = res;
            out.rank = tok.rank;
            return out;
        }
    }
    // No map suffix: a diffuse map named after the whole stem.
    out.baseId = stem;
    out.kind = TextureMapKind::diffuse;
    out.recognized = false;
    out.resolutionK = 0;
    return out;
}

auto list_texture_folder_files(const String& folder, bool recursive)
    -> Vector<String> {
    namespace fs = std::filesystem;
    Vector<String> out;
    std::error_code ec;
    if (!fs::is_directory(folder, ec)) {
        return out;
    }
    auto add = [&](const fs::directory_entry& e) {
        std::error_code ec2;
        if (e.is_regular_file(ec2)) {
            out.push_back(fs::relative(e.path(), folder, ec2).generic_string());
        }
    };
    if (recursive) {
        for (const auto& e : fs::recursive_directory_iterator(folder, ec)) {
            add(e);
        }
    } else {
        for (const auto& e : fs::directory_iterator(folder, ec)) {
            add(e);
        }
    }
    std::sort(out.begin(), out.end());
    return out;
}

auto expand_texture_atlas_config(const Json& cfg, const String& textureDir,
                                 const TextureFileLister& lister) -> Json {
    Json out = cfg;
    out.erase("defaults");
    if (!cfg.contains("content") || !cfg.at("content").is_array()) {
        return out;
    }
    const Json defaults = cfg.contains("defaults") && cfg.at("defaults").is_object()
                              ? cfg.at("defaults")
                              : Json::object();
    static const std::set<String> kEntryOnly = {"folder", "custom", "recursive", "max_res", "exclude"};

    Json content = Json::array();
    for (const Json& item : cfg.at("content")) {
        if (!item.contains("folder")) {
            // Explicit entry: the defaults are its lowest priority fields.
            Json e = Json::object();
            merge_into(e, defaults, {"max_res"});
            merge_into(e, item, {});
            content.push_back(std::move(e));
            continue;
        }

        const String folder = normalize_folder(item.at("folder").get<String>());
        const bool recursive = item.value("recursive", false);
        Json base = Json::object();
        merge_into(base, defaults, kEntryOnly);
        merge_into(base, item, kEntryOnly);
        I32 maxRes = 0;
        if (defaults.contains("max_res")) {
            maxRes = parse_res_limit(defaults.at("max_res"));
        }
        if (item.contains("max_res")) {
            maxRes = parse_res_limit(item.at("max_res"));
        }
        const Json custom = item.contains("custom") && item.at("custom").is_object()
                                ? item.at("custom")
                                : Json::object();
        std::set<String> excluded;
        if (item.contains("exclude") && item.at("exclude").is_array()) {
            for (const auto& x : item.at("exclude")) {
                excluded.insert(x.get<String>());
            }
        }

        const String absFolder = get_path(textureDir, folder);
        const auto files = lister(absFolder, recursive);
        if (files.empty()) {
            logWARN("Texture folder '{}' has no file (does '{}' exist?).", folder, absFolder);
        }

        // Group the files by texture.
        std::map<String, Group> groups; // key: subdir/baseId
        for (const String& rel : files) {
            String dir;
            String fname = rel;
            const size_t slash = rel.rfind('/');
            if (slash != String::npos) {
                dir = rel.substr(0, slash);
                fname = rel.substr(slash + 1);
            }
            String stem, ext;
            split_ext(fname, stem, ext);
            const I32 extRank = image_ext_rank(ext);
            if (extRank < 0 || stem.empty()) {
                continue;
            }
            const TextureFileName parsed = parse_texture_file_name(stem);
            if (parsed.kind == TextureMapKind::ignored) {
                logDEBUG("Texture folder '{}': '{}' ignored (unused map).", folder, rel);
                continue;
            }
            const String key = dir.empty() ? parsed.baseId : dir + "/" + parsed.baseId;
            auto& grp = groups[key];
            grp.subdir = dir;
            grp.baseId = parsed.baseId;
            Candidate cand{rel, parsed.resolutionK, parsed.rank, extRank};
            auto it = grp.maps.find(parsed.kind);
            if (it == grp.maps.end() || better(cand, it->second, maxRes)) {
                grp.maps[parsed.kind] = cand;
            }
        }

        std::set<String> usedCustom;
        U32 count = 0;
        for (const auto& [key, grp] : groups) { // sorted by key: deterministic
            if (excluded.count(key) != 0) {
                continue;
            }
            if (grp.maps.count(TextureMapKind::diffuse) == 0) {
                logWARN("Texture folder '{}': '{}' has no diffuse map, skipped.", folder, key);
                continue;
            }
            const String id = folder + "/" + key;
            Json e = base;
            e["id"] = id;
            for (const auto& [kind, cand] : grp.maps) {
                e[key_of(kind)] = folder + "/" + cand.rel;
            }

            const Json* cust = nullptr;
            if (custom.contains(key)) {
                cust = &custom.at(key);
                usedCustom.insert(key);
            } else if (custom.contains(id)) {
                cust = &custom.at(id);
                usedCustom.insert(id);
            }
            if (cust != nullptr && cust->is_object()) {
                for (auto it = cust->begin(); it != cust->end(); ++it) {
                    const String& k = it.key();
                    const bool isMap = std::find(std::begin(kMapKeys), std::end(kMapKeys), k) != std::end(kMapKeys);
                    e[k] = isMap && it.value().is_string() ? Json(folder + "/" + it.value().get<String>())
                                                           : it.value();
                }
            }
            for (const char* need : {"type", "category", "dimsM"}) {
                NVCHK(e.contains(need),
                      "Texture folder '{}': '{}' has no '{}' (set it in `defaults` or in the folder entry).",
                      folder, id, need);
            }
            content.push_back(std::move(e));
            ++count;
        }
        for (auto it = custom.begin(); it != custom.end(); ++it) {
            if (usedCustom.count(it.key()) == 0) {
                logWARN("Texture folder '{}': custom entry '{}' matches no texture.", folder, it.key());
            }
        }
        logINFO("Texture folder '{}': {} texture(s).", folder, count);
    }
    out["content"] = std::move(content);
    return out;
}

auto load_texture_atlas_desc(const Json& cfg, const String& textureDir,
                             const TextureFileLister& lister) -> CellTextureAtlasDesc {
    const Json expanded = expand_texture_atlas_config(cfg, textureDir, lister);
    CellTextureAtlasDesc desc = expanded.get<CellTextureAtlasDesc>();
    return desc;
}

} // namespace nv
