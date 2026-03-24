
#ifndef NV_RESOURCEPROVIDER_
#define NV_RESOURCEPROVIDER_

#include <nvk_common.h>

namespace nv {

class ResourceProvider : public RefObject {
  public:
    using ReadCallback = std::function<void(String data, const String& error)>;

    virtual auto list_files() -> Vector<String> = 0;
    virtual auto contains_file(const String& fileName) -> bool = 0;
    virtual auto get_file_metadata(const String& fileName) -> Json = 0;
    virtual auto get_version() const -> I64 { return 0; }
    virtual auto get_name() const -> String = 0;

    virtual auto supports_sync_read() const -> bool { return false; }
    virtual auto read_file(const String& fileName) -> String {
        THROW_MSG("Sync read not supported by provider {}", get_name());
        return {};
    }

    virtual void read_file_async(const String& fileName,
                                 ReadCallback callback) = 0;
};

} // namespace nv

#endif
