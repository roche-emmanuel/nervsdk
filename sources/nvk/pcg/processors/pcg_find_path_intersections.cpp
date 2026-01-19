
#include <nvk_pcg.h>

namespace nv {

/** Find intersections from all the input paths. */
void pcg_find_path_2d_intersections(PCGContext& ctx) {

    auto& in = ctx.inputs();

    auto paths = in.get_raw_slot("In").as_vector<RefPtr<PointArray>>();
    logDEBUG("Processing {} input paths.", paths.size());

    F64 endPointDist = in.get("EndPointSnapDistance", 0.0);

    // We generate the polylines from the provided paths:
    Polyline2Vector<F64> lines;
    I32 idx = 0;

    for (const auto& path : paths) {
        Polyline2<F64> line;
        line.id = idx;
        line.closedLoop = false;

        const auto& pos = path->get_position_attribute();

        // Add all the points from this path:
        if (pos.is_type<Vec3d>()) {
            const auto& arr = pos.get_values<Vec3d>();
            for (const auto& p : arr) {
                line.points.emplace_back(p.x(), p.y());
            }
        } else if (pos.is_type<Vec2d>()) {
            line.points = pos.get_values<Vec2d>();
        }

        lines.emplace_back(std::move(line));
        idx++;
    }

    // Compute the 2D intersections:
    auto results = compute_polyline2_intersections(lines, endPointDist);

    // In the process we also need to collect the attribute types to recreate
    // them on the output:

    auto adescs = PointArray::collect_all_attribute_types(paths);

    // Collect the intersection results:
    U32 nIntersecs =
        results.intersections.size() + results.endpointNearSegments.size();
    auto outPoints = PointArray::create(adescs, nIntersecs);

    // We must already have the position attribute now.
    // auto& posArr = outPoints->add_attribute<Vec3d>(pt_position_attr);
    // auto& posArr = outPoints->get<Vec3d>(pt_position_attr);
    auto& seg0LineArr = outPoints->add_attribute<I32>("line0_index");
    auto& seg0IdxArr = outPoints->add_attribute<I32>("seg0_index");
    auto& seg1LineArr = outPoints->add_attribute<I32>("line1_index");
    auto& seg1IdxArr = outPoints->add_attribute<I32>("seg1_index");
    auto& intersecTypeArr = outPoints->add_attribute<I32>("intersect_type");

    auto interpolate_point = [&paths](I32 pathId, I32 segId,
                                      const Vec2d& ipos) {
        auto& path = paths[pathId];
        auto startPt = path->get_point(segId);
        auto endPt = path->get_point(
            segId == (path->get_num_points() - 1) ? 0 : (segId + 1));

        // Compute our interpolation ratio:
        auto startPos = startPt.position().xy();
        auto endPos = endPt.position().xy();
        auto ratio = startPos == endPos ? 0.0
                                        : (ipos - startPos).length() /
                                              (endPos - startPos).length();

        //   Ratio should be in range [0,1]
        NVCHK(0.0 <= ratio && ratio <= 1.0,
              "Unexpected interpolation ratio: {}", ratio);

        // Create a new point and interpolate there:
        return PCGPoint::mix(startPt, endPt, ratio);
    };

    I32 i = 0;
    for (const auto& intersec : results.intersections) {
        auto pathId0 = intersec.s0.lineId;
        auto segId0 = intersec.s0.index;

        auto pathId1 = intersec.s1.lineId;
        auto segId1 = intersec.s1.index;

        auto pt0 = interpolate_point(pathId0, segId0, intersec.position);
        auto pt1 = interpolate_point(pathId1, segId1, intersec.position);

        // Mix the point into the output array:
        auto outPt = outPoints->get_point(i);
        outPt.mix_from(pt0, pt1, 0.5);

        seg0LineArr[i] = pathId0;
        seg0IdxArr[i] = segId0;
        seg1LineArr[i] = pathId1;
        seg1IdxArr[i] = segId1;
        intersecTypeArr[i] = ITYPE_4WAY;
        ++i;
    }

    // Now handle the 3way intersections:
    for (const auto& nearSeg : results.endpointNearSegments) {
        // Get the end point details:
        auto endPoint =
            paths[nearSeg.pathId]->get_point(nearSeg.isStart ? 0 : -1);

        // Get the alternate segment points:
        const auto& path = paths[nearSeg.segment.lineId];
        auto segId = nearSeg.segment.index;
        auto pt0 = path->get_point(segId);
        auto pt1 = path->get_point(
            segId == (path->get_num_points() - 1) ? 0 : (segId + 1));

        F64 t = (nearSeg.intersection - pt0.position().xy()).length() /
                (pt1.position().xy() - pt0.position().xy()).length();

        // // Next we need to project our endPoint onto the segment:
        // F64 t = 0.0;
        // seg2_project_point(pt0.position().xy(), pt1.position().xy(),
        //                    endPoint.position().xy(), &t);
        NVCHK(0 <= t && t <= 1.0,
              "Expected projection to be on segment: t = {} ", t);
        auto ptA = PCGPoint::mix(pt0, pt1, t);

        auto outPt = outPoints->get_point(i);
        outPt.mix_from(ptA, endPoint, 0.5);

        // Replace the position of the output point as this one should be left
        // on the segment:
        outPt.set_position(ptA.position());

        seg0LineArr[i] = nearSeg.pathId;
        seg0IdxArr[i] = (I32)endPoint.index();
        seg1LineArr[i] = nearSeg.segment.lineId;
        seg1IdxArr[i] = nearSeg.segment.index;
        intersecTypeArr[i] = ITYPE_3WAY;
        ++i;
    }

    ctx.outputs().set("Out", outPoints);
}

} // namespace nv