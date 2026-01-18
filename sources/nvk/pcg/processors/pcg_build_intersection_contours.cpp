
#include <nvk_pcg.h>

#include <algorithm>

namespace nv {

namespace {

auto get_segment_dir(PointArrayVector& paths, I32 pId, I32 segId) -> Vec2d {
    auto pt0 = paths[pId]->get_seg_start_point(segId);
    auto pt1 = paths[pId]->get_seg_end_point(segId);
    return (pt1.position().xy() - pt0.position().xy()).normalized();
};

auto get_intersection_min_angle(const Vec2d& dir0, const Vec2d& dir1) -> F64 {
    auto angle = dir0.angleTo(dir1);
    // If angle is larger than PI/2 returh the complement to PI:
    return angle > PI_2 ? (PI - angle) : angle;
}

void sort_ccw(Vector<Vec2d>& points) {
    std::ranges::sort(points, [](const Vec2d& a, const Vec2d& b) {
        double angleA = std::atan2(a.y(), a.x());
        double angleB = std::atan2(b.y(), b.x());

        return angleA < angleB;
    });
}

auto handle_4way_intersection(PCGContext& ctx, PCGPointRef& iPt,
                              PointArrayVector& outPaths) {
    auto& in = ctx.inputs();
    PointArrayVector& paths = in.get("In");

    // We retreive the 4 points of the intersecting segments:
    auto dir0 = get_segment_dir(paths, iPt.get<I32>("line0_index"),
                                iPt.get<I32>("seg0_index"));
    auto dir1 = get_segment_dir(paths, iPt.get<I32>("line1_index"),
                                iPt.get<I32>("seg1_index"));

    // Compute the minimal angle between the dirs:
    auto angle = get_intersection_min_angle(dir0, dir1);

    // Also get the road half width value:
    F64 roadWidth = in.get("RoadWidth", 500.0);
    F64 halfWidth = roadWidth * 0.5;

    // We also need a min distance between sibling roads on a turn:
    F64 minSpacing = in.get("TurnMinSpacing", 200.0);
    F64 halfSpacing = minSpacing * 0.5;

    // Half of the min angle should be used to cover the road half width and
    // half of the spacing distance:
    // tan(angle*0.5) = (halfWidth+halfSpacing) / L
    F64 L = (halfWidth + halfSpacing) / std::tan(angle * 0.5);

    // L represents the minimal distance we need to cut the road paths from to
    // generate the road intersection. So starting from the intersection center
    // we place those points on the 4 directions we have:
    auto center = iPt.position().xy();

    // Note: for now we can assume that the center is at (0,0) for the
    // computations, and translate at the end.
    Vector<Vec2d> mainPoints = {dir0 * L, dir1 * L, -dir0 * L, -dir1 * L};
    sort_ccw(mainPoints);

    // Prepare a new output array with the number of points we need:
    // Num points per spline:
    I32 spNum = in.get("TurnSplineResolution", 20);

    F64 spTension = in.get("TurnSplineTension", 1.0);

    auto path = PointArray::create(spNum * 4);
    path->add_std_attributes();

    // next we add the transition points from one road to the next one:
    // Vector<Vec2d> points;
    I32 idx = 0;
    for (I32 i = 0; i < 4; ++i) {
        auto& pt0 = mainPoints[i];
        auto& pt1 = mainPoints[i == 3 ? 0 : i + 1];

        auto xdir = pt0.normalized();
        auto ydir = xdir.ccw90();

        // path->get_point(idx).set_position(Vec3d(center + pt0, 0.0));

        // Construct the spline2d :
        auto startPos = pt0 + ydir * halfWidth;
        auto xdir1 = pt1.normalized();
        auto ydir1 = xdir1.ccw90();
        auto endPos = pt1 - ydir1 * halfWidth;

        Spline2d sp({{startPos, -xdir * spTension, -xdir * spTension},
                     {endPos, xdir1 * spTension, xdir1 * spTension}});

        for (I32 j = 0; j < spNum; ++j) {
            F64 t = F64(j) / F64(spNum - 1);
            auto pos = sp.evaluate(t);
            path->get_point(idx++).set_position(Vec3d(center + pos, 0.0));
        }
    }

    outPaths.emplace_back(path);
};

} // namespace

/** Find intersections from all the input paths. */
void pcg_build_intersection_contours(PCGContext& ctx) {

    auto& out = ctx.outputs();

    // First we compute the intersections:
    pcg_find_path_2d_intersections(ctx);

    // Get the raw intersection points:
    RefPtr<PointArray> rawIntersections = out.get("Out");
    U32 nIntersecs = rawIntersections->get_num_points();

    auto handle_3way_intersection = [](PCGPointRef& iPt) {

    };

    PointArrayVector outPaths;

    // Process each intersection:
    for (U32 i = 0; i < nIntersecs; ++i) {
        auto iPoint = rawIntersections->get_point(i);

        // handle the intersection:
        auto itype = iPoint.get<I32>("intersect_type");

        switch (itype) {
        case ITYPE_4WAY:
            handle_4way_intersection(ctx, iPoint, outPaths);
            break;
        case ITYPE_3WAY:
            handle_3way_intersection(iPoint);
            break;
        default:
            break;
        }
    }

    // store the output paths:
    ctx.outputs().set("Out", outPaths, true);
}

} // namespace nv