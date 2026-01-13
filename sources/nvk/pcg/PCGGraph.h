#ifndef _NV_PCGGRAPH_H_
#define _NV_PCGGRAPH_H_

#include <nvk_common.h>

namespace nv {

class PCGGraph : public RefObject {
    NV_DECLARE_NO_COPY(PCGGraph)
    NV_DECLARE_NO_MOVE(PCGGraph)

  public:
    PCGGraph();
    ~PCGGraph() override;
};

}; // namespace nv

#endif
