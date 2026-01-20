
#include <nvk_pcg.h>

#include <clipper2/clipper.h>

namespace nv {

/**
Compute the contours around paths given an expand distance.
*/
void pcg_compute_path_offsets(PCGContext& ctx) {

    auto& in = ctx.inputs();
    PointArrayVector& arrays = in.get("In");

    F64 distance = in.get("Distance");
    NVCHK(distance != 0.0, "Invalid distance.");

    // Process all paths together:
    Clipper2Lib::PathsD cPaths;
    cPaths.reserve(arrays.size());

    I32 closed = -1;

    for (const auto& path : arrays) {
        U32 num = path->get_num_points();
        if (num == 0) {
            continue;
        }

        Clipper2Lib::PathD cPath;
        cPath.resize(num);

        I32 newClosed = path->is_closed_loop() ? 1 : 0;
        if (closed == -1) {
            closed = newClosed;
        } else {
            NVCHK(closed == newClosed,
                  "pcg_compute_path_offsets: Mixing closed/non closed paths.");
        }

        for (U32 i = 0; i < num; ++i) {
            auto pos = path->get_point(i).position().xy();
            cPath[i] = {pos.x(), pos.y()};
        }

        cPaths.emplace_back(std::move(cPath));
    }

    PointArrayVector contours;

    if (closed == -1) {
        // No output to provide:
        ctx.outputs().set("Out", contours);
        return;
    }

    Clipper2Lib::PathsD solution = Clipper2Lib::InflatePaths(
        cPaths, distance, Clipper2Lib::JoinType::Round,
        closed == 1 ? Clipper2Lib::EndType::Polygon
                    : Clipper2Lib::EndType::Round);

    for (const auto& sol : solution) {
        if (sol.empty()) {
            continue;
        }

        RefPtr<PointArray> contour = PointArray::create((I32)sol.size());
        auto& positions = contour->add_attribute<Vec3d>(pt_position_attr);

        // Copy the 2D point data:
        for (U32 i = 0; i < sol.size(); ++i) {
            positions[i].set(sol[i].x, sol[i].y, 0.0);
        }

        // Setting this contour as closed loop:
        contour->set_closed_loop(true);

        contours.emplace_back(contour);
    }

    // Write the output:
    ctx.outputs().set("Out", contours);
}

} // namespace nv