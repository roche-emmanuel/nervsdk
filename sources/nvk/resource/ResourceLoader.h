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

    auto get_resource(const char* resName) -> RefPtr<RefObject>;

  protected:
    virtual auto load_resource(const char* fullpath) -> RefPtr<RefObject> = 0;

    Vector<String> _paths;

    UnorderedMap<StringID, RefPtr<RefObject>> _loadedResources;
};

} // namespace nv

#endif
