
#include <nvk_pcg.h>

namespace nv {

namespace {
auto get_path_2D_length(const RefPtr<PointArray>& path) -> F64 {
    U32 num = path->get_num_points();
    if (num <= 1)
        return 0.0;

    auto pt0 = path->get_point(0);
    F64 total = 0.0;
    for (I32 i = 1; i < num; ++i) {
        auto pt1 = path->get_point(i);
        total += (pt1.position().xy() - pt0.position().xy()).length();
        pt0 = pt1;
    }

    if (path->is_closed_loop()) {
        auto pt1 = path->get_point(0);
        total += (pt1.position().xy() - pt0.position().xy()).length();
    }

    return total;
}

auto resample_path(const RefPtr<PointArray>& path, I32 numPoints, F64 distance)
    -> RefPtr<PointArray> {
    auto arr = PointArray::create_like(path, numPoints);
    auto pt0 = path->get_point(0);

    F64 targetLength = 0.0;
    F64 curBaseLength = 0.0;
    I32 segIdx = 0;
    I32 nSegs = (I32)path->get_num_segments();

    for (I32 i = 0; i < numPoints; ++i) {
        targetLength = i * distance;
        auto pt1 = path->get_seg_end_point(segIdx);
        auto segLength = (pt1.position().xy() - pt0.position().xy()).length();

        while ((curBaseLength + segLength) < targetLength && segIdx < (nSegs-1)) {
            // move to then next segment:
            pt0 = pt1;
            segIdx++;
            // NVCHK(segIdx < nSegs, "Out of range path segment index: {}",
            //       segIdx);

            pt1 = path->get_seg_end_point(segIdx);
            // Move the current base length:
            curBaseLength += segLength;

            // Compute the new segment length:
            segLength = (pt1.position().xy() - pt0.position().xy()).length();
        }

        // we are on the proper segment to resample this point, so we get the t
        // parameter:
        F64 t = (targetLength - curBaseLength) / segLength;
        // NVCHK(0.0 <= t && t <= 1.0, "Unexpected t parameter value: {}", t);
        t = std::clamp(t, 0.0, 1.0);

        arr->set_point(i, PCGPoint::mix(pt0, pt1, t));
    }

    return arr;
}

}; // namespace

/** Add the data id attribute to the input arrays. */
void pcg_resample_paths(PCGContext& ctx) {

    auto& in = ctx.inputs();
    PointArrayVector& arrays = in.get("In");

    F64 distanceHint = in.get("DistanceHint");
    bool fitToCurve = in.get("FitToCurve");
    NVCHK(distanceHint > 0.0, "Invalid distance hint.");
    NVCHK(fitToCurve, "Expected fit to curve = true for now.");

    // Process each path separately:
    PointArrayVector resampledPaths;
    for (const auto& path : arrays) {
        // Get the full path length:
        F64 totalLength = get_path_2D_length(path);
        I32 numPoints = 1 + (I32)std::round(0.5 + totalLength / distanceHint);
        F64 distance = totalLength / F64(numPoints - 1);

        auto arr = resample_path(path, numPoints, distance);
        resampledPaths.emplace_back(arr);
    }

    // Next write the same list of arrays as output:
    ctx.outputs().set("Out", resampledPaths);
}

} // namespace nv