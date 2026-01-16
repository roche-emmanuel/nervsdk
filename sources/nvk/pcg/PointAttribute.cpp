#include <nvk/pcg/PointAttribute.h>

namespace nv {
PointAttribute::PointAttribute(String name, Traits traits)
    : _traits(std::move(traits)), _name(std::move(name)) {};
PointAttribute::~PointAttribute() = default;

void PointAttribute::randomize_values(const Box4d& range) {
    switch (get_type_id()) {
    case DTYPE_I32:
        randomize_values(I32(range.xmin), I32(range.xmax));
        break;
    case DTYPE_I64:
        randomize_values(I64(range.xmin), I64(range.xmax));
        break;
    case DTYPE_F32:
        randomize_values(F32(range.xmin), F32(range.xmax));
        break;
    case DTYPE_F64:
        randomize_values(F64(range.xmin), F64(range.xmax));
        break;
    case DTYPE_VEC2D:
        randomize_values(range.minimum().xy(), range.maximum().xy());
        break;
    case DTYPE_VEC3D:
        randomize_values(range.minimum().xyz(), range.maximum().xyz());
        break;
    case DTYPE_VEC4D:
        randomize_values(range.minimum(), range.maximum());
        break;
    default:
        THROW_MSG("unsupported data type to randomize: {}", get_type_id());
    }
}

} // namespace nv
