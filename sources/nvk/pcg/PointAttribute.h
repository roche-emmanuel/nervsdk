#ifndef _NV_POINTATTRIBUTE_H_
#define _NV_POINTATTRIBUTE_H_

#include <nvk_common.h>

namespace nv {

class PointAttribute : public RefObject {
    NV_DECLARE_NO_COPY(PointAttribute)
    NV_DECLARE_NO_MOVE(PointAttribute)
  public:
    struct Traits {};

    explicit PointAttribute(String name, Traits traits);
    ~PointAttribute() override;

    virtual auto size() const -> U64 = 0;
    virtual auto element_size() const -> U32 = 0;

    auto name() const -> const String& { return _name; }

  protected:
    Traits _traits;
    String _name;
};

using PointAttributeVector = Vector<RefPtr<PointAttribute>>;

template <typename T> class TypedPointAttribute : public PointAttribute {
  public:
    TypedPointAttribute(String name, U32 size, const T& value, Traits traits)
        : PointAttribute(std::move(name), traits), _values(size, value) {};
    TypedPointAttribute(String name, Vector<T> values, Traits traits)
        : PointAttribute(std::move(name), std::move(traits)),
          _values(std::move(values)) {};

    auto get_values() -> Vector<T>& { return _values; }
    auto get_values() const -> const Vector<T>& { return _values; }

    auto size() const -> U64 override { return _values.size(); }
    auto element_size() const -> U32 override { return sizeof(T); }

    static auto create(String name, U32 size, const T& value,
                       Traits traits = {}) -> RefPtr<TypedPointAttribute<T>> {
        return nv::create<TypedPointAttribute<T>>(std::move(name), size, value,
                                                  std::move(traits));
    }
    static auto create(String name, Vector<T> values, Traits traits = {})
        -> RefPtr<TypedPointAttribute<T>> {
        return nv::create<TypedPointAttribute<T>>(
            std::move(name), std::move(values), std::move(traits));
    }

  protected:
    Vector<T> _values;
};

using F32PointAttribute = TypedPointAttribute<F32>;
using F64PointAttribute = TypedPointAttribute<F64>;
using Vec3fPointAttribute = TypedPointAttribute<Vec3f>;
using Vec3dPointAttribute = TypedPointAttribute<Vec3d>;

}; // namespace nv

#endif
