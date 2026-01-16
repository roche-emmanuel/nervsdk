#ifndef _NV_POINTARRAY_H_
#define _NV_POINTARRAY_H_

#include <nvk/pcg/PointAttribute.h>
#include <nvk_common.h>

#include <nvk_type_ids.h>

namespace nv {

static constexpr const char* pt_index_attr = "$Index";
static constexpr const char* pt_position_attr = "$Position";
static constexpr const char* pt_rotation_attr = "$Rotation";
static constexpr const char* pt_scale_attr = "$Scale";
static constexpr const char* pt_boundsmin_attr = "$BoundsMin";
static constexpr const char* pt_boundsmax_attr = "$BoundsMax";
static constexpr const char* pt_color_attr = "$Color";
static constexpr const char* pt_density_attr = "$Density";
static constexpr const char* pt_steepness_attr = "$Steepness";
static constexpr const char* pt_seed_attr = "$Seed";

class PointArray : public RefObject {
    NV_DECLARE_NO_COPY(PointArray)
    NV_DECLARE_NO_MOVE(PointArray)

  public:
    struct Traits {};

    struct AttribDesc {
        String name;
        StringID type;
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

    static auto
    collect_all_attribute_types(const Vector<RefPtr<PointArray>>& arrays)
        -> Vector<AttribDesc>;

    auto clone() const -> RefPtr<PointArray>;

    void collect_attribute_types(PointAttributeTypeMap& atypes);

    /** Retrieve the number of attributes. */
    auto get_num_attributes() const -> U32;
    auto get_num_points() const -> U32;

    // Get all attribute names
    [[nodiscard]] auto get_attribute_names() const -> Vector<String>;

    auto find_attribute(const String& name) const -> const PointAttribute*;
    auto find_attribute(const String& name) -> PointAttribute*;

    auto get_attribute(const String& name) const -> const PointAttribute&;
    auto get_attribute(const String& name) -> PointAttribute&;

    template <typename T> auto find(const String& name) -> Vector<T>* {
        const auto* attr = find_attribute(name);
        if (attr->is_type<T>()) {
            return &attr->get_values<T>();
        }
        return nullptr;
    };
    template <typename T> auto get(const String& name) -> Vector<T>& {
        return get_attribute(name).get_values<T>();
    };

    void add_std_attributes();

    auto get_position_attribute() const -> const PointAttribute&;

    auto get_rotation_attribute() const -> const PointAttribute&;

    auto get_scale_attribute() const -> const PointAttribute&;

    void add_attributes(const Vector<AttribDesc>& attribs);
    void add_attribute(RefPtr<PointAttribute> attr);

    /** Randomize all attribute values. */
    void randomize_all_attributes(UnorderedMap<String, Box4d> ranges = {});

    template <typename T>
    auto add_attribute(const String& name, T&& initValue = {})
        -> Vector<std::decay_t<T>>& {
        using ValueType = std::decay_t<T>;
        auto size = get_num_points();
        auto attr = PointAttribute::create<ValueType>(
            name, size, std::forward<T>(initValue));
        add_attribute(attr);
        return attr->template get_values<ValueType>();
    }

    void resize(U32 size) {
        _numPoints = (I32)size;
        for (const auto& it : _attributes) {
            it.second->resize(size);
        }
    }

    auto get_attributes() const -> const PointAttributeMap&;

    // Get a reference to a point (modifications affect the array)
    auto get_point(U64 index) -> PCGPointRef;

    // Get a const reference to a point
    auto get_point(U64 index) const -> PCGPointRef;

    // Get a copy of a point (modifications don't affect the array)
    auto copy_point(U64 index) const -> PCGPoint;

    // Set a point's values from a Point object
    void set_point(U64 index, const PCGPoint& point);

  protected:
    Traits _traits;
    PointAttributeMap _attributes;
    I32 _numPoints{-1};

    void validate_attributes();
};

using PointArrayVector = Vector<RefPtr<PointArray>>;

}; // namespace nv

#endif
