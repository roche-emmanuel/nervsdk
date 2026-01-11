#ifndef _NV_POINTDATASET_H_
#define _NV_POINTDATASET_H_

#include <nvk/pcg/PointAttribute.h>
#include <nvk_common.h>

namespace nv {

class PointDataset : public RefObject {
    NV_DECLARE_NO_COPY(PointDataset)
    NV_DECLARE_NO_MOVE(PointDataset)

  public:
    PointDataset();
    ~PointDataset() override;

  protected:
    UnorderedMap<String, RefPtr<PointAttribute>> _attributes;
};

}; // namespace nv

#endif
