#ifndef NV_RESOURCELOADER_
#define NV_RESOURCELOADER_

#include <nvk/base/std_containers.h>

namespace nv {

class ResourceLoader : public RefObject {
    // NV_DECLARE_CLASS(ResourceLoader)
    NV_DECLARE_NO_COPY(ResourceLoader)
    NV_DECLARE_NO_MOVE(ResourceLoader)

  public:
    // Constructor:
    ResourceLoader();

    // Destructor
    ~ResourceLoader() override;

    /**
     * Returns the path of the resource of the given name.
     *
     * @param name the name of a resource.
     * @return the path of this resource.
     * @throw exception if the resource is not found.
     */
    virtual auto find_resource(const char* name, String& fullpath) -> bool;

    auto add_path(const char* path) -> bool;

    /** Remove a given search path from the list */
    auto remove_path(const char* path) -> bool;

    auto get_resource(const char* resName) -> RefPtr<RefObject> {
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

  protected:
    virtual auto load_resource(const char* fullpath) -> RefPtr<RefObject> = 0;

    Vector<String> _paths;

    UnorderedMap<StringID, RefPtr<RefObject>> _loadedResources;
};

} // namespace nv

#endif
