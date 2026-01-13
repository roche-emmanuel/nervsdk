#ifndef _NV_PCGCONTEXT_H_
#define _NV_PCGCONTEXT_H_

#include <nvk/base/SlotMap.h>

namespace nv {

class PCGContext : public RefObject {
    NV_DECLARE_NO_COPY(PCGContext)
    NV_DECLARE_NO_MOVE(PCGContext)

  public:
    struct Traits {};

    explicit PCGContext(Traits traits);
    ~PCGContext() override;

    static auto create(Traits traits = {}) -> RefPtr<PCGContext>;

    auto inputs() -> SlotMap& { return *_inputs; }
    auto outputs() -> SlotMap& { return *_outputs; }

  protected:
    Traits _traits;
    RefPtr<SlotMap> _inputs;
    RefPtr<SlotMap> _outputs;
};

}; // namespace nv

#endif
