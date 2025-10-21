#ifndef NV_RESOURCEMANAGER_
#define NV_RESOURCEMANAGER_

#include <nvk/base/Signal.h>
#include <nvk/base/std_containers.h>
#include <nvk/resource/ResourceLoader.h>
#include <nvk/resource/ResourcePacker.h>

namespace nv {

class ResourceManager {
    NV_DECLARE_CUSTOM_INSTANCE(ResourceManager)

  public:
    virtual ~ResourceManager() = default;

    /** Get the last update time from a set of files. */
    virtual auto get_last_update_time(const Set<String>& files)
        -> std::time_t = 0;

    virtual auto check_live_reload() -> bool = 0;

    virtual auto get_root_path() -> String = 0;

    /** Check if we have a given resource pack */
    [[nodiscard]] auto has_resource_pack(const String& packFile) const -> bool;

    /** Add a given resource pack */
    void add_resource_pack(const String& packFile);

    /** Check if a virtual file exist */
    [[nodiscard]] auto virtual_file_exists(const String& fname,
                                           bool forceAllowSystem = false) const
        -> bool;

    /** Add a specific resource type path */
    void add_resource_location(StringID category, const String& rpath);

    /** Search for a given resource path */
    auto search_resource_path(StringID category, String& full_path,
                              const char* filename = nullptr) -> bool;

    /** validate a resource path*/
    auto validate_resource_path(StringID category, String& full_path)
        -> String&;

    auto validate_resource_path(StringID category, const char* filename)
        -> String;

    auto read_virtual_file(const String& fname, bool forceAllowSystem = false)
        -> String;
    auto read_virtual_binary_file(const String& fname,
                                  bool forceAllowSystem = false) -> U8Vector;

    auto get_file_last_write_time(const String& fname) -> std::time_t;

    template <typename F> void on_resources_ready(F&& func) {
        if (_dirtyResourcePacks) {
            resourcesReady.connect(std::forward<F>(func));
        } else {
            // Execute immediately;
            func();
        }
    }

    void register_resource_packs(const StringVector& packFiles);

    Signal<> resourcesReady;

  protected:
    /** flag to allow system files usage. */
    bool _useSystemFiles{true};

    /** Encryption keys */
    U8Vector _aesKey;
    U8Vector _aesIV;

  private:
    /** List of resource unpackers */
    Vector<RefPtr<ResourceUnpacker>> _unpackers;

    /** List of configurable paths per category*/
    UnorderedMap<StringID, StringVector> _allResourcePaths;

    /** flag for resource dirty state */
    bool _dirtyResourcePacks{true};
};

} // namespace nv

#endif
