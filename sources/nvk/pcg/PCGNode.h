#ifndef _NV_PCGNODE_H_
#define _NV_PCGNODE_H_

#include <nvk_common.h>

namespace nv {

class PCGNode : public RefObject {
    NV_DECLARE_NO_COPY(PCGNode)
    NV_DECLARE_NO_MOVE(PCGNode)

  public:
    PCGNode();
    ~PCGNode() override;
};

}; // namespace nv

#endif
