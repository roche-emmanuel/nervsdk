
#ifndef NV_RESOURCEPROVIDER_
#define NV_RESOURCEPROVIDER_

#include <nvk_common.h>

namespace nv {

class Task;

class ResourceProvider : public RefObject {
  public:
    // List all files in the pack
    virtual auto list_files() -> Vector<String> = 0;

    // Check if a file exists in the pack
    virtual auto contains_file(const String& fileName) -> bool = 0;

    // Get file metadata
    virtual auto get_file_metadata(const String& fileName) -> Json = 0;

    virtual auto read_file(const String& fileName) -> RefPtr<Task> = 0;
};

} // namespace nv

#endif
