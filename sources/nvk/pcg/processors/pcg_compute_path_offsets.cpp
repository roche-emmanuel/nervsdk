
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
    NVCHK(distance > 0.0, "Invalid distance.");

    // Process all paths together:
    Clipper2Lib::PathsD cPaths;

    for (const auto& path : arrays) {
        U32 num = path->get_num_points();
        if (num == 0) {
            continue;
        }

        Clipper2Lib::PathD cPath;
        cPath.resize(path->is_closed_loop() ? num + 1 : num);

        for (I32 i = 0; i < num; ++i) {
            auto pos = path->get_point(i).position().xy();
            cPath[i] = {pos.x(), pos.y()};
        }

        if (path->is_closed_loop()) {
            auto pos = path->get_point(0).position().xy();
            cPath[num] = {pos.x(), pos.y()};
        }
        cPaths.emplace_back(std::move(cPath));
    }

    Clipper2Lib::PathsD solution = Clipper2Lib::InflatePaths(
        cPaths, distance, Clipper2Lib::JoinType::Round,
        Clipper2Lib::EndType::Round);

    PointArrayVector contours;
    for (const auto& sol : solution) {
        if (sol.empty()) {
            continue;
        }

        RefPtr<PointArray> contour = PointArray::create((I32)sol.size());
        U32 ptSize = sizeof(Clipper2Lib::PointD);
        auto& positions = contour->add_attribute<Vec3d>(pt_position_attr);

        if (ptSize == sizeof(Vec2d)) {
            // Copy the 2D point data:
            for (U32 i = 0; i < sol.size(); ++i) {
                positions[i].set(sol[i].x, sol[i].y, 0.0);
            }
        } else if (ptSize == sizeof(Vec3d)) {
            auto& positions = contour->add_attribute<Vec3d>(pt_position_attr);
            // Copy the 2D point data:
            memcpy(positions.data(), sol.data(), sol.size() * ptSize);
        }

        // Setting this contour as closed loop (but not sure this is a good
        // idea actually ?)
        contour->set_closed_loop(true);

        contours.emplace_back(contour);
    }

    // Write the output:
    ctx.outputs().set("Out", contours);
}

} // namespace nv