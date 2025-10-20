#ifndef NV_STDLOGGER_H
#define NV_STDLOGGER_H

#include <log/LogSink.h>

namespace nv {

class StdLogger : public LogSink {
  public:
    void output(int level, const char* prefix, const char* msg,
                size_t size) override;
};

} // namespace nv

#endif
