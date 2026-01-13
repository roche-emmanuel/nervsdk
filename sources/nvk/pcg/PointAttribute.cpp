#include <nvk/pcg/PointAttribute.h>

namespace nv {
PointAttribute::PointAttribute(String name, Traits traits)
    : _traits(std::move(traits)), _name(std::move(name)) {};
PointAttribute::~PointAttribute() = default;
} // namespace nv
