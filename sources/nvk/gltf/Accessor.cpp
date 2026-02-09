#include <nvk_gltf.h>

namespace nv {
GLTFAccessor::GLTFAccessor(GLTFAsset& parent, U32 index)
    : GLTFElement(parent, index) {}
auto GLTFAccessor::name() const -> const String& { return _name; }
void GLTFAccessor::set_name(String name) { _name = std::move(name); }
auto GLTFAccessor::component_type() const -> GLTFComponentType {
    return _componentType;
}
void GLTFAccessor::set_component_type(GLTFComponentType type) {
    _componentType = type;
}
auto GLTFAccessor::normalized() const -> bool { return _normalized; }
void GLTFAccessor::set_normalized(bool normalized) { _normalized = normalized; }
auto GLTFAccessor::element_type() const -> GLTFElementType {
    return _elementType;
}
void GLTFAccessor::set_element_type(GLTFElementType type) {
    _elementType = type;
}
auto GLTFAccessor::offset() const -> U32 { return _offset; }
void GLTFAccessor::set_offset(U32 offset) { _offset = offset; }
auto GLTFAccessor::count() const -> U32 { return _count; }
void GLTFAccessor::set_count(U32 count) { _count = count; }
void GLTFAccessor::set_buffer_view(GLTFBufferView& view) {
    _bufferView = &view;
}
auto GLTFAccessor::has_min() const -> bool { return _hasMin; }
auto GLTFAccessor::min() const -> const F32Vector& { return _min; }
void GLTFAccessor::set_min(const F32Vector& min) {
    _min = min;
    _hasMin = true;
}
void GLTFAccessor::clear_min() { _hasMin = false; }
auto GLTFAccessor::has_max() const -> bool { return _hasMax; }
auto GLTFAccessor::max() const -> const F32Vector& { return _max; }
void GLTFAccessor::set_max(const F32Vector& max) {
    _max = max;
    _hasMax = true;
}
void GLTFAccessor::clear_max() { _hasMax = false; }
void GLTFAccessor::read(const Json& desc) {
    // Optional name
    if (desc.contains("name")) {
        _name = desc["name"].get<String>();
    }

    // Required componentType
    _componentType =
        static_cast<GLTFComponentType>(desc.at("componentType").get<U32>());

    // Optional normalized (defaults to false)
    _normalized = desc.value("normalized", false);

    // Required type (as string: "SCALAR", "VEC2", etc.)
    const String typeStr = desc.at("type").get<String>();
    _elementType = gltf::to_element_type(typeStr);

    // Required count
    _count = desc.at("count").get<U32>();

    // Optional bufferView
    if (desc.contains("bufferView")) {
        const U32 bufferViewIndex = desc["bufferView"].get<U32>();
        _bufferView = &_parent.get_bufferview(bufferViewIndex);
    }

    // Optional byteOffset (defaults to 0)
    _offset = desc.value("byteOffset", 0U);

    // Optional min values
    _min.clear();
    if (desc.contains("min")) {
        const auto& minArray = desc["min"];
        _hasMin = true;
        for (size_t i = 0; i < minArray.size() && i < 16; ++i) {
            _min.push_back(minArray[i].get<F32>());
        }
    } else {
        _hasMin = false;
    }

    // Optional max values
    _max.clear();
    if (desc.contains("max")) {
        const auto& maxArray = desc["max"];
        _hasMax = true;
        for (size_t i = 0; i < maxArray.size() && i < 16; ++i) {
            _max.push_back(maxArray[i].get<F32>());
        }
    } else {
        _hasMax = false;
    }
}
auto GLTFAccessor::write() const -> Json {
    Json desc;

    // Required fields
    desc["componentType"] = static_cast<U32>(_componentType);
    desc["type"] = std::string(gltf::to_string(_elementType));
    desc["count"] = _count;

    // Optional fields
    if (!_name.empty()) {
        desc["name"] = _name;
    }

    if (_normalized) {
        desc["normalized"] = _normalized;
    }

    if (_bufferView != nullptr) {
        desc["bufferView"] = _bufferView->index();
    }

    if (_offset != 0) {
        desc["byteOffset"] = _offset;
    }

    // Min/Max arrays
    if (_hasMin) {
        // Json minArray = Json::array();
        const size_t numComponents =
            gltf::get_element_component_count(_elementType);
        NVCHK(numComponents == _min.size(), "Unexpected _min size.");
        // for (size_t i = 0; i < numComponents; ++i) {
        //     minArray.push_back(_min[i]);
        // }
        // desc["min"] = std::move(minArray);
        desc["min"] = _min;
    }

    if (_hasMax) {
        // Json maxArray = Json::array();
        const size_t numComponents =
            gltf::get_element_component_count(_elementType);
        NVCHK(numComponents == _max.size(), "Unexpected _max size.");
        // for (size_t i = 0; i < numComponents; ++i) {
        //     maxArray.push_back(_max[i]);
        // }
        // desc["max"] = std::move(maxArray);
        desc["max"] = _max;
    }

    return desc;
}

auto GLTFAccessor::data() -> U8* {
    NVCHK(_bufferView != nullptr, "Invalid bufferView to get data.");
    return _bufferView->data() + _offset;
}

auto GLTFAccessor::data() const -> const U8* {
    NVCHK(_bufferView != nullptr, "Invalid bufferView to get data.");
    return _bufferView->data() + _offset;
};

void GLTFAccessor::update_bounds() {
    if (get_data_type() != DTYPE_VEC3F) {
        // Can only compute the bounds with Vec3f data.
        return;
    }

    U32 stride = _bufferView->stride();
    NVCHK(stride > 0, "Invalid bufferview stride.");
    U8* ptr = data();
    Box3f bb;

    for (I32 i = 0; i < _count; ++i) {
        bb.extendTo(*((Vec3f*)ptr));
        ptr += stride;
    }
    set_min(bb.minimum());
    set_max(bb.maximum());
};

auto GLTFAccessor::get_data_type() const -> DataType {
    return gltf::get_data_type(_elementType, _componentType);
};

void GLTFAccessor::set_min(const Vec3f& vec) {
    _hasMin = true;
    _min = {vec.x(), vec.y(), vec.z()};
};
void GLTFAccessor::set_max(const Vec3f& vec) {
    _hasMax = true;
    _max = {vec.x(), vec.y(), vec.z()};
};

} // namespace nv