
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
        if (pos.is_type<Vec3f>()) {
            const auto& arr = pos.get_values<Vec3f>();
            for (const auto& p : arr) {
                line.points.emplace_back(p.x(), p.y());
            }
        } else if (pos.is_type<Vec2f>()) {
            line.points = pos.get_values<Vec2f>();
        }

        lines.emplace_back(std::move(line));
        idx++;
    }

    // Compute the 2D intersections:
    auto results = compute_polyline2_intersections(lines, 0.01F);

    // Collect the intersection results:
    auto outPoints = PointArray::create(results.intersections.size());

    auto& posArr = outPoints->add_attribute<Vec3f>(pt_position_attr);
    auto& seg0LineArr = outPoints->add_attribute<I32>("seg0_line_index");
    auto& seg0IdxArr = outPoints->add_attribute<I32>("seg0_index");
    auto& seg1LineArr = outPoints->add_attribute<I32>("seg1_line_index");
    auto& seg1IdxArr = outPoints->add_attribute<I32>("seg1_index");

    I32 i = 0;
    for (const auto& intersec : results.intersections) {
        // TODO: should compute an interpolated Z value for the position below:
        posArr[i].set(intersec.position.x(), intersec.position.y(), 0.0);
        seg0LineArr[i] = intersec.s0.lineId;
        seg0IdxArr[i] = intersec.s0.index;
        seg1LineArr[i] = intersec.s1.lineId;
        seg1IdxArr[i] = intersec.s1.index;
        ++i;
    }

    ctx.outputs().set("Out", outPoints);
}

} // namespace nv