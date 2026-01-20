
#include <nvk_pcg.h>

namespace nv {

namespace {
auto compute_path_contours(const RefPtr<PointArray>& path, F64 distance)
    -> RefPtr<PointArray> {
    RefPtr<PointArray> contour = PointArray::create();

    return contour;
}

} // namespace

/**
Compute the contours around paths given an expand distance.
*/
void pcg_compute_path_contours(PCGContext& ctx) {

    auto& in = ctx.inputs();
    PointArrayVector& arrays = in.get("In");

    F64 distance = in.get("Distance");
    NVCHK(distance > 0.0, "Invalid distance.");

    // Process each path separately:
    PointArrayVector contours;
    for (const auto& path : arrays) {
        contours.emplace_back(compute_path_contours(path, distance));
    }

    // Write the output:
    ctx.outputs().set("Out", contours);
}

} // namespace nv