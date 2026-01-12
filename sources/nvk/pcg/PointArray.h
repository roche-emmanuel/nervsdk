#ifndef _NV_POINTARRAY_H_
#define _NV_POINTARRAY_H_

#include <nvk/pcg/PointAttribute.h>
#include <nvk_common.h>

namespace nv {

class PointArray : public RefObject {
    NV_DECLARE_NO_COPY(PointArray)
    NV_DECLARE_NO_MOVE(PointArray)

  public:
    PointArray();
    ~PointArray() override;

  protected:
    UnorderedMap<String, RefPtr<PointAttribute>> _attributes;
};

using PointArrayVector = Vector<RefPtr<PointArray>>;

}; // namespace nv

#endif
