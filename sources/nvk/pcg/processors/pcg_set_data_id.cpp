
#include <nvk_pcg.h>

namespace nv {

/** Add the data id attribute to the input arrays. */
void pcg_set_data_id(PCGContext& ctx) {

    auto& in = ctx.inputs();
    PointArrayVector& arrays = in.get("In");

    // Get attribute name:
    String attribName = in.get("AttribName", "dataId");
    bool inplace = in.get("InPlace", true);

    // Iterate on the arrays in place only for now:
    U32 num = arrays.size();
    PointArrayVector outs;
    for (I32 i = 0; i < num; ++i) {
        auto arr = inplace ? arrays[i] : arrays[i]->clone();
        arr->add_attribute(attribName, i);
        outs.emplace_back(arr);
    }

    // Next write the same list of arrays as output:
    ctx.outputs().set("Out", outs);
}

} // namespace nv