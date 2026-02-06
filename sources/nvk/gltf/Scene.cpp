#include <nvk_gltf.h>

namespace nv {
GLTFScene::GLTFScene(GLTFAsset& parent, U32 index)
    : GLTFElement(parent, index) {}
auto GLTFScene::name() const -> const String& { return _name; }
void GLTFScene::set_name(String name) { _name = std::move(name); }
auto GLTFScene::nodes_count() const -> U32 {
    return static_cast<U32>(_nodes.size());
}
auto GLTFScene::nodes() const -> const Vector<RefPtr<GLTFNode>>& {
    return _nodes;
}
auto GLTFScene::nodes() -> Vector<RefPtr<GLTFNode>>& { return _nodes; }
auto GLTFScene::get_node(U32 index) const -> const GLTFNode& {
    return *_nodes[index];
}
auto GLTFScene::get_node(U32 index) -> GLTFNode& { return *_nodes[index]; }
void GLTFScene::add_node(GLTFNode& node) { _nodes.emplace_back(&node); }
void GLTFScene::clear_nodes() { _nodes.clear(); }
void GLTFScene::read(const Json& desc) {
    if (desc.contains("name") && desc["name"].is_string()) {
        _name = desc["name"].get<String>();
    }

    if (desc.contains("nodes") && desc["nodes"].is_array()) {
        const auto& nodes = desc["nodes"];
        _nodes.reserve(nodes.size());
        for (const auto& nodeIndex : nodes) {
            U32 idx = nodeIndex.get<U32>();
            _nodes.emplace_back(&_parent.get_node(idx));
        }
    }
}
auto GLTFScene::write() const -> Json {
    Json json;

    if (!_name.empty()) {
        json["name"] = _name;
    }

    if (!_nodes.empty()) {
        Json nodes = Json::array();
        for (const auto& node : _nodes) {
            nodes.push_back(node->index());
        }
        json["nodes"] = nodes;
    }

    return json;
}
} // namespace nv