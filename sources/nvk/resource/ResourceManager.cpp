// Implementation for ResourceManager

#include <nvk/resource/ResourceManager.h>

namespace nv {

NV_IMPLEMENT_CUSTOM_INSTANCE(ResourceManager)

ResourceManager::ResourceManager() = default;

void ResourceManager::uninit_instance() {
    _allResourcePaths.clear();
    _unpackers.clear();
}

void ResourceManager::init_instance() {}

void ResourceManager::add_resource_pack(const String& packFile) {
    NVCHK(!has_resource_pack(packFile), "Resource pack {} already loaded.",
          packFile);

    NVCHK(system_file_exists(packFile), "Resource file {} doesn't exist.",
          packFile);

    auto up = nv::create<ResourceUnpacker>(packFile, _aesKey, _aesIV);
    _unpackers.push_back(up);
}

auto ResourceManager::has_resource_pack(const String& packFile) const -> bool {
    for (const auto& up : _unpackers) {
        if (up->get_filename() == packFile) {
            return true;
        }
    }

    return false;
}

void ResourceManager::add_resource_location(StringID category,
                                            const String& rpath) {
    _allResourcePaths[category].push_back(rpath);
}

auto ResourceManager::search_resource_path(StringID category, String& full_path,
                                           const char* filename) -> bool {
    if (filename == nullptr) {
        NVCHK(!full_path.empty(), "No sub path provided.");
        filename = full_path.c_str();
    }

    // Issue #51: Also test with prepending the rootPath is use system files is
    // enabled. This is helpfull for instance in WASM build of legacy
    // TerrainView apps.
    const auto& rpath = get_root_path();
    bool isAbsolute = is_absolute_path(filename);
    if (_useSystemFiles && !isAbsolute) {
        String file = get_path(rpath, filename);
        // logDEBUG("Checking file {}...", file);
        if (system_file_exists(file)) {
            full_path = file;
            // logDEBUG("Found file {}.", full_path);
            return true;
        }
    }

    // logDEBUG("Checking file {}...", filename);
    if (virtual_file_exists(filename)) {
        full_path = filename;
        // logDEBUG("Found file {}.", full_path);
        return true;
    }

    if (isAbsolute) {
        return false;
    }

    StringVector& paths = _allResourcePaths[category];
    if (_useSystemFiles) {
        for (auto& res_path : paths) {
            String f_path = get_path(rpath, res_path, filename);
            // logDEBUG("Checking file {}...", f_path);
            if (system_file_exists(f_path)) {
                full_path = f_path;
                // logDEBUG("Found file {}.", full_path);
                return true;
            }
        }
    }

    for (auto& res_path : paths) {
        String f_path = get_path(res_path, filename);
        // logDEBUG("Checking file {}...", f_path);
        if (virtual_file_exists(f_path)) {
            full_path = f_path;
            // logDEBUG("Found file {}.", full_path);
            return true;
        }
    }

    return false;
}

auto ResourceManager::virtual_file_exists(const String& fname,
                                          bool forceAllowSystem) const -> bool {
    if (_useSystemFiles && system_file_exists(fname)) {
        // Search for a real file first as this will be overriding the packs
        // content:
        return true;
    }

    // logDEBUG("Searching for file {}", fname);

    // Check for files in the resource packs:
    for (const auto& up : _unpackers) {
        if (up->contains_file(fname)) {
            // logDEBUG("Found file {} in pack {}", fname,
            // up->get_filename());
            return true;
        }
    }

    return false;
}

auto ResourceManager::read_virtual_file(const String& fname,
                                        bool forceAllowSystem) -> String {

    if ((_useSystemFiles || forceAllowSystem)) {
        if (system_file_exists(fname)) {
            return read_system_file(fname.c_str());
        }

        String f_path = get_path(get_root_path(), fname);
        if (system_file_exists(f_path)) {
            return read_system_file(f_path.c_str());
        }
    }

    // Check for files in the resource packs:
    for (const auto& up : _unpackers) {
        if (up->contains_file(fname)) {
            return up->extract_file_as_string(fname);
        }
    }

    THROW_MSG("Cannot read virtual file {}", fname);
    return {};
}

auto ResourceManager::read_virtual_binary_file(const String& fname,
                                               bool forceAllowSystem)
    -> U8Vector {

    if ((_useSystemFiles || forceAllowSystem) && system_file_exists(fname)) {
        return read_system_binary_file(fname.c_str());
    }

    // Check for files in the resource packs:
    for (const auto& up : _unpackers) {
        if (up->contains_file(fname)) {
            return up->extract_file(fname);
        }
    }

    THROW_MSG("Cannot read virtual file {}", fname);
    return {};
}

auto ResourceManager::validate_resource_path(StringID category,
                                             const char* filename) -> String {
    String full_path = filename;
    return validate_resource_path(category, full_path);
}

auto ResourceManager::validate_resource_path(StringID category,
                                             String& full_path) -> String& {
    bool res = search_resource_path(category, full_path);
    NVCHK(res, "Cannot find valid file for resource {}", full_path.c_str());
    return full_path;
}

static auto get_system_file_last_write_time(const String& fname)
    -> std::time_t {
    auto currentFileTime = std::filesystem::last_write_time(fname);
    auto fileEpoch = currentFileTime.time_since_epoch();
    auto systemTime = std::chrono::system_clock::time_point(
        std::chrono::duration_cast<std::chrono::system_clock::duration>(
            fileEpoch));
    // The following is not supported in emscripten v3.1.65 (?)
    // auto systemTime =
    //     std::chrono::clock_cast<std::chrono::system_clock>(currentFileTime);
    return std::chrono::system_clock::to_time_t(systemTime);
}

auto ResourceManager::get_file_last_write_time(const String& fname)
    -> std::time_t {
    if (_useSystemFiles && system_file_exists(fname)) {
        return get_system_file_last_write_time(fname);
    }

    // Iterate on the data packs:
    for (const auto& up : _unpackers) {
        if (up->contains_file(fname)) {
            auto packFile = up->get_filename();
            // Note: we must have a real file for this pack for now.
            // Eventually we might support "packs into packs", but
            // we would need to add support for a "string buffer" instead
            // of the file stream object in the unpacker first.
            return get_system_file_last_write_time(packFile);
        }
    }

    THROW_MSG("File {} not found.", fname);
    return 0;
}

void ResourceManager::register_resource_packs(const StringVector& packFiles) {
    logDEBUG("Loading {} resource packs", packFiles.size());
    for (const auto& packFile : packFiles) {
        logDEBUG("Loading resource pack {}...", packFile);
        add_resource_pack(packFile);
    }

    // When done registering the resource packs we trigger the ready signal:
    _dirtyResourcePacks = false;
    resourcesReady.emit();
}

} // namespace nv
