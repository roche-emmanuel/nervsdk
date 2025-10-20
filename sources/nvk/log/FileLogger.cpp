
#include <nvk/log/FileLogger.h>
#include <nvk/log/LogManager.h>

namespace nv {

void FileLogger::output(int /*level*/, const char* prefix, const char* msg,
                        size_t /*size*/) {
    if (prefix != nullptr) {
        _stream << prefix;
    }
    // _stream.write(msg, (std::streamsize)size) << std::endl;
    _stream << msg << std::endl;
    _stream.flush();
}

FileLogger::FileLogger(const char* filename, bool append) {
    _stream.open(filename, append ? (std::ofstream::out | std::ofstream::app)
                                  : std::ofstream::out);
    NVCHK(_stream.is_open(), "Cannot open file {}", filename);
}

FileLogger::~FileLogger() { _stream.close(); }

} // namespace nv
