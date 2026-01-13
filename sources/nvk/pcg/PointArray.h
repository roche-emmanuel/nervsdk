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

    explicit PointArray(Traits traits);
    PointArray(PointAttributeVector attribs, Traits traits);
    ~PointArray() override;

    static auto create(Traits traits) -> RefPtr<PointArray>;
    static auto create(PointAttributeVector attribs, Traits traits = {})
        -> RefPtr<PointArray>;

    /** Retrieve the number of attributes. */
    auto get_num_attributes() const -> U32;

    auto find_attribute(const String& name) const -> const PointAttribute*;

    auto get_attribute(const String& name) const -> const PointAttribute&;

    auto get_position_attribute() const -> const PointAttribute&;

    auto get_rotation_attribute() const -> const PointAttribute&;

    auto get_scale_attribute() const -> const PointAttribute&;

    // void add_attribute(RefPtr<PointAttribute> attrib);

  protected:
    Traits _traits;
    PointAttributeVector _attributes;

    void validate_attributes();
};

using PointArrayVector = Vector<RefPtr<PointArray>>;

NV_DEFINE_REFPTR_TYPE_ID(nv::PointArray);

}; // namespace nv

#endif
