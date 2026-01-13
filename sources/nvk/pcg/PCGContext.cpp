#include <nvk/pcg/PCGContext.h>

namespace nv {
PCGContext::PCGContext(Traits traits) : _traits(std::move(traits)) {};

PCGContext::~PCGContext() = default;

auto PCGContext::create(Traits traits) -> RefPtr<PCGContext> {
    return nv::create<PCGContext>(std::move(traits));
}
} // namespace nv
