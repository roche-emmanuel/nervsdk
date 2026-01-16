
#include <nvk_pcg.h>

namespace nv {

/** Add the data id attribute to the input arrays. */
void pcg_set_data_id(PCGContext& ctx) {

    PointArrayVector& arrays = ctx.inputs().get("In");

    // Get attribute name:
    String attribName = ctx.inputs().get("AttribName", "dataId");

    // Iterate on the arrays in place only for now:
    U32 num = arrays.size();
    for (I32 i = 0; i < num; ++i) {
        auto& arr = arrays[i];
        arr->add_attribute(attribName, i);
    }

    // Next write the same list of arrays as output:
    ctx.outputs().set("Out", arrays);
}

} // namespace nv