
#include <nvk_pcg.h>

namespace nv {

/** Find intersections from all the input paths. */
void pcg_find_path_2d_intersections(PCGContext& ctx) {

    auto& in = ctx.inputs().get_raw_slot("In");

    auto paths = in.as_vector<RefPtr<PointArray>>();
    logDEBUG("Processing {} input paths.", paths.size());

    // We generate the polylines from the provided paths:
    Polyline2Vector<F32> lines;
    I32 idx = 0;
    for (const auto& path : paths) {

        Polyline2<F32> line;
        line.id = idx;
        line.closedLoop = false;

        const auto& pos = path->get_position_attribute();

        // Add all the points from this path:
        // if (pos.is<Vec3f>()) {

        // }
        // else if (path.has)
        //     path.get_attribute("position");

        // line.points.push_back(Vec2f(0.0f, 0.0f));
        // line.points.push_back(Vec2f(1.0f, 0.0f));
        idx++;
    }
}

} // namespace nv