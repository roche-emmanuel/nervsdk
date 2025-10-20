#include <log/StdLogger.h>

namespace nv {

void StdLogger::output(int /*level*/, const char* prefix, const char* msg,
                       size_t /*size*/) {
    if (prefix != nullptr) {
        std::cout << prefix;
    }

    // std::cout.write(msg, (std::streamsize)size) << std::endl;
    std::cout << msg << std::endl;
    std::cout.flush();
}

} // namespace nv
