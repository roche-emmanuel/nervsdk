#ifndef _NV_POINTATTRIBUTE_H_
#define _NV_POINTATTRIBUTE_H_

#include <nvk_common.h>

namespace nv {

class PointAttribute : public RefObject {
    NV_DECLARE_NO_COPY(PointAttribute)
    NV_DECLARE_NO_MOVE(PointAttribute)
  public:
    PointAttribute();
    ~PointAttribute() override;

    virtual auto size() const -> U64 = 0;
    virtual auto element_size() const -> U32 = 0;
};

template <typename T> class TypedPointAttribute : public PointAttribute {
  public:
    auto get_values() -> Vector<T>& { return _values; }
    auto get_values() const -> const Vector<T>& { return _values; }

    auto size() const -> U64 override { return _values.size(); }
    auto element_size() const -> U32 override { return sizeof(T); }

  protected:
    Vector<T> _values;
};

using F32PointAttribute = TypedPointAttribute<F32>;
using F64PointAttribute = TypedPointAttribute<F64>;
using Vec3fPointAttribute = TypedPointAttribute<Vec3f>;
using Vec3dPointAttribute = TypedPointAttribute<Vec3d>;

}; // namespace nv

#endif
