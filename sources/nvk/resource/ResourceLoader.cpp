// Implementation for ResourceLoader

#include <nvk/resource/ResourceLoader.h>
namespace nv {

ResourceLoader::ResourceLoader() {
    logTRACE("Creating ResourceLoader object.");
}

ResourceLoader::~ResourceLoader() {
    logTRACE("Deleting ResourceLoader object.");
}

auto ResourceLoader::find_resource(const char* name, String& fullpath) -> bool {
    // We iterate on each available path searching for the resourse file:
    String fpath;
    for (auto& p : _paths) {
        fpath = p + name;
        if (system_file_exists(fpath.c_str())) {
            fullpath = fpath;
            return true;
        }
    }

    return false;
}

auto ResourceLoader::add_path(const char* path) -> bool {
    // Check if the path ends with a separator:
    String newpath = path;
    std::replace(newpath.begin(), newpath.end(), '\\', '/');
    replace_all(newpath, "//", "/");

    if (newpath.back() != '/') {
        newpath += '/';
    }

    // We should not add a path multiple times, so we ensure here it is not
    // already in the list:
    if (find(_paths.begin(), _paths.end(), newpath) != _paths.end()) {
        logTRACE("Resource path {} already registered.", newpath);
        return false;
    }

    logTRACE("Adding resource search path: '{}'", newpath);
    _paths.push_back(newpath);
    return true;
}

auto ResourceLoader::remove_path(const char* path) -> bool {
    auto it = find(_paths.begin(), _paths.end(), path);
    if (it != _paths.end()) {
        _paths.erase(it);
        return true;
    }

    return false;
}

auto ResourceLoader::get_resource(const char* resName) -> RefPtr<RefObject> {
    // Check if the resource is already loaded:
    StringID id = SID(resName);
    auto it = _loadedResources.find(id);
    if (it != _loadedResources.end()) {
        return it->second;
    }

    // Otherwise we need to load the resource data.
    // So we first need to get the full path to the resource:
    String fullpath;
    if (!find_resource(resName, fullpath)) {
        logERROR("Cannot find resource {}", resName);
        return {};
    }

    // We have a fullpath, so we load the corresponding resource:
    logTRACE("Loading resource from file: {}", fullpath.c_str());
    RefPtr<RefObject> res = load_resource(fullpath.c_str());
    NVCHK(res != nullptr, "Cannot load resource for {}", resName);

    // We have a valid resource object, so we should store it:
    // Note: we only use the resource name when computing the ID,
    // not the full path.
    _loadedResources.insert(std::make_pair(id, res));

    return res;
}
} // namespace nv
