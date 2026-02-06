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
auto GLTFAccessor::stride() const -> U32 { return _stride; }
void GLTFAccessor::set_stride(U32 stride) { _stride = stride; }
void GLTFAccessor::set_buffer_view(GLTFBufferView& view) {
    _bufferView = &view;
}
auto GLTFAccessor::has_min() const -> bool { return _hasMin; }
auto GLTFAccessor::min() const -> const std::array<F32, 16>& { return _min; }
void GLTFAccessor::set_min(const std::array<F32, 16>& min) {
    _min = min;
    _hasMin = true;
}
void GLTFAccessor::clear_min() { _hasMin = false; }
auto GLTFAccessor::has_max() const -> bool { return _hasMax; }
auto GLTFAccessor::max() const -> const std::array<F32, 16>& { return _max; }
void GLTFAccessor::set_max(const std::array<F32, 16>& max) {
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
    if (desc.contains("min")) {
        const auto& minArray = desc["min"];
        _hasMin = true;
        for (size_t i = 0; i < minArray.size() && i < 16; ++i) {
            _min[i] = minArray[i].get<F32>();
        }
    } else {
        _hasMin = false;
    }

    // Optional max values
    if (desc.contains("max")) {
        const auto& maxArray = desc["max"];
        _hasMax = true;
        for (size_t i = 0; i < maxArray.size() && i < 16; ++i) {
            _max[i] = maxArray[i].get<F32>();
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
        Json minArray = Json::array();
        const size_t numComponents =
            gltf::get_element_component_count(_elementType);
        for (size_t i = 0; i < numComponents; ++i) {
            minArray.push_back(_min[i]);
        }
        desc["min"] = std::move(minArray);
    }

    if (_hasMax) {
        Json maxArray = Json::array();
        const size_t numComponents =
            gltf::get_element_component_count(_elementType);
        for (size_t i = 0; i < numComponents; ++i) {
            maxArray.push_back(_max[i]);
        }
        desc["max"] = std::move(maxArray);
    }

    return desc;
}

} // namespace nv