#include <nvk/pcg/PointArray.h>

namespace nv {
PointArray::PointArray(Traits traits) : _traits(std::move(traits)) {};

PointArray::~PointArray() = default;

auto PointArray::create(Traits traits) -> RefPtr<PointArray> {
    return nv::create<PointArray>(std::move(traits));
}

PointArray::PointArray(PointAttributeVector attribs, Traits traits)
    : _traits(std::move(traits)), _attributes(std::move(attribs)) {
    validate_attributes();
};

void PointArray::validate_attributes() {
    if (_attributes.size() <= 1) {
        // Nothing to validate in this case:
        return;
    }

    // Check that all attributes have the same length:
    U32 size = _attributes[0]->size();
    U32 num = _attributes.size();
    for (U32 i = 1; i < num; ++i) {
        U32 size2 = _attributes[i]->size();
        if (size != size2) {
            THROW_MSG("Mismatch in attribute {} num points: {} != {}",
                      _attributes[i]->name(), size2, size);
        }
    }
};

auto PointArray::create(PointAttributeVector attribs, Traits traits)
    -> RefPtr<PointArray> {
    return nv::create<PointArray>(std::move(attribs), std::move(traits));
}

auto PointArray::get_num_attributes() const -> U32 {
    return _attributes.size();
}

} // namespace nv
