#ifndef _NV_POINTARRAY_H_
#define _NV_POINTARRAY_H_

#include <nvk/pcg/PointAttribute.h>
#include <nvk_common.h>

namespace nv {

static constexpr const char* pt_position_attr = "position";
static constexpr const char* pt_rotation_attr = "rotation";
static constexpr const char* pt_scale_attr = "scale";

class PointArray : public RefObject {
    NV_DECLARE_NO_COPY(PointArray)
    NV_DECLARE_NO_MOVE(PointArray)

  public:
    struct Traits {};

    struct AttribDesc {
        String name;
        I32 type;
    };

    explicit PointArray(Traits traits);
    PointArray(const PointAttributeVector& attribs, Traits traits);
    ~PointArray() override;

    static auto create(I32 numPoints = -1, Traits traits = {})
        -> RefPtr<PointArray>;
    static auto create(const PointAttributeVector& attribs, Traits traits = {})
        -> RefPtr<PointArray>;

    static auto create(const Vector<AttribDesc>& attribs, U32 numPoints,
                       Traits traits = {}) -> RefPtr<PointArray>;

    /** Retrieve the number of attributes. */
    auto get_num_attributes() const -> U32;
    auto get_num_points() const -> U32;

    auto find_attribute(const String& name) const -> const PointAttribute*;

    auto get_attribute(const String& name) const -> const PointAttribute&;

    template <typename T>
    auto get_data(const String& name) const -> Vector<T>& {
        return get_attribute(name).get_values<T>();
    };

    auto get_position_attribute() const -> const PointAttribute&;

    auto get_rotation_attribute() const -> const PointAttribute&;

    auto get_scale_attribute() const -> const PointAttribute&;

    void add_attribute(RefPtr<PointAttribute> attr);

    template <typename T>
    auto add_attribute(const String& name, T&& initValue = {}) -> Vector<T>& {
        auto size = get_num_points();
        auto attr =
            PointAttribute::create<T>(name, size, std::forward<T>(initValue));
        add_attribute(attr);
        return attr->template get_values<T>();
    }

    void resize(U32 size) {
        _numPoints = (I32)size;
        for (const auto& it : _attributes) {
            it.second->resize(size);
        }
    }

    // void add_attribute(RefPtr<PointAttribute> attrib);

  protected:
    Traits _traits;
    PointAttributeMap _attributes;
    I32 _numPoints{-1};

    void validate_attributes();
};

using PointArrayVector = Vector<RefPtr<PointArray>>;

NV_DEFINE_REFPTR_TYPE_ID(nv::PointArray);

}; // namespace nv

#endif
