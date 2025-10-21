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

} // namespace nv
