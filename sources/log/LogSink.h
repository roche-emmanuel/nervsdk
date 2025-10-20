#ifndef NV_LOGSINK_H
#define NV_LOGSINK_H

#include <nvk_common.h>

#include <base/RefObject.h>

namespace nv {

class LogSink : public nv::RefObject {
  public:
    virtual void output(int level, const char* prefix, const char* msg,
                        size_t size) = 0;
};

} // namespace nv

#endif
