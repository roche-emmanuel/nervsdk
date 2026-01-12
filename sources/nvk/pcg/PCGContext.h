#ifndef _NV_PCGCONTEXT_H_
#define _NV_PCGCONTEXT_H_

#include <nvk/pcg/PointAttribute.h>
#include <nvk_common.h>

namespace nv {

class PCGContext : public RefObject {
    NV_DECLARE_NO_COPY(PCGContext)
    NV_DECLARE_NO_MOVE(PCGContext)

  public:
    PCGContext();
    ~PCGContext() override;
};

}; // namespace nv

#endif
