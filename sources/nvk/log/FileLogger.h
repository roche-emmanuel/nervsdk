#ifndef NV_FILELOGGER_H_
#define NV_FILELOGGER_H_

#include <nvk/log/LogSink.h>

namespace nv {

class FileLogger : public LogSink {
    NV_DECLARE_NO_COPY(FileLogger)
    NV_DECLARE_NO_MOVE(FileLogger)

  public:
    explicit FileLogger(const char* filename, bool append = false);

    ~FileLogger() override;

    /**
    Output a given message on the LogSink object.
    */
    void output(int level, const char* prefix, const char* msg,
                size_t size) override;

  private:
    /** File output stream object.*/
    std::ofstream _stream;
};

} /* namespace nv*/

#endif /* FILELOGGER_H_ */
