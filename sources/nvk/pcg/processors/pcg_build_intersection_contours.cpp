
#include <nvk_pcg.h>

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

struct IntersectionConfig {
    Vector<Vec2d> mainPoints;
    Vector<I32> splineSegments; // num points per segment
    F64 spTension;
    F64 spPower;
};

auto compute_intersection_config(PCGContext& ctx, const Vec2d& dir0,
                                 const Vec2d& dir1, bool is4Way)
    -> IntersectionConfig {
    auto& in = ctx.inputs();

    auto angle = get_intersection_min_angle(dir0, dir1);
    F64 roadWidth = in.get("RoadWidth", 500.0);
    F64 halfWidth = roadWidth * 0.5;
    F64 minSpacing = in.get("TurnMinSpacing", 200.0);
    F64 halfSpacing = minSpacing * 0.5;
    F64 L = (halfWidth + halfSpacing) / std::tan(angle * 0.5);

    IntersectionConfig config;

    if (is4Way) {
        config.mainPoints = {dir0 * L, dir1 * L, -dir0 * L, -dir1 * L};
        sort_ccw(config.mainPoints);
        config.splineSegments =
            Vector<I32>(4, in.get("TurnSplineResolution", 20));

    } else {
        config.mainPoints = {dir0 * L};
        if (dir0.signedAngleTo(dir1) > 0.0) {
            config.mainPoints.push_back(dir1 * L);
            config.mainPoints.push_back(-dir1 * L);
        } else {
            config.mainPoints.push_back(-dir1 * L);
            config.mainPoints.push_back(dir1 * L);
        }
        I32 spNum = in.get("TurnSplineResolution", 20);
        config.splineSegments = {spNum, 2, spNum}; // middle segment is straight
    }

    config.spTension = in.get("TurnTensionScale", 80.0);
    config.spPower = in.get("TurnTensionPower", 3.2);

    return config;
}

auto get_intersection_directions(PointArrayVector& paths, PCGPointRef& iPt,
                                 bool is4Way) -> std::pair<Vec2d, Vec2d> {
    if (is4Way) {
        auto dir0 = get_segment_dir(paths, iPt.get<I32>("line0_index"),
                                    iPt.get<I32>("seg0_index"));
        auto dir1 = get_segment_dir(paths, iPt.get<I32>("line1_index"),
                                    iPt.get<I32>("seg1_index"));
        return {dir0, dir1};
    }

    auto endPt = paths[iPt.get<I32>("line0_index")]->get_point(
        iPt.get<I32>("seg0_index"));
    auto dir0 = (endPt.position().xy() - iPt.position().xy()).normalized();
    auto dir1 = get_segment_dir(paths, iPt.get<I32>("line1_index"),
                                iPt.get<I32>("seg1_index"));
    return {dir0, dir1};
}

auto build_intersection_path(const Vec2d& center,
                             const IntersectionConfig& config, F64 halfWidth)
    -> RefPtr<PointArray> {
    I32 totalPoints = std::accumulate(config.splineSegments.begin(),
                                      config.splineSegments.end(), 0);
    auto path = PointArray::create(totalPoints);
    path->add_std_attributes();

    I32 idx = 0;
    I32 numSegments = (I32)config.mainPoints.size();

    for (I32 i = 0; i < numSegments; ++i) {
        const auto& pt0 = config.mainPoints[i];
        const auto& pt1 = config.mainPoints[(i + 1) % numSegments];

        auto xdir = pt0.normalized();
        auto ydir = xdir.ccw90();
        auto startPos = pt0 + ydir * halfWidth;

        auto xdir1 = pt1.normalized();
        auto ydir1 = xdir1.ccw90();
        auto endPos = pt1 - ydir1 * halfWidth;

        auto tension =
            config.spTension * std::pow(xdir.angleTo(xdir1), config.spPower);
        Spline2d sp({{startPos, -xdir * tension, -xdir * tension},
                     {endPos, xdir1 * tension, xdir1 * tension}});

        I32 num = config.splineSegments[i];
        for (I32 j = 0; j < num; ++j) {
            F64 t = F64(j) / F64(num - 1);
            auto pos = sp.evaluate(t);
            path->get_point(idx++).set_position(Vec3d(center + pos, 0.0));
        }
    }

    return path;
}

auto handle_intersection(PCGContext& ctx, PCGPointRef& iPt,
                         PointArrayVector& outPaths, bool is4Way) {
    auto& in = ctx.inputs();
    PointArrayVector& paths = in.get("In");

    auto [dir0, dir1] = get_intersection_directions(paths, iPt, is4Way);
    auto config = compute_intersection_config(ctx, dir0, dir1, is4Way);

    F64 roadWidth = in.get("RoadWidth", 500.0);
    F64 halfWidth = roadWidth * 0.5;
    auto center = iPt.position().xy();

    auto path = build_intersection_path(center, config, halfWidth);
    outPaths.emplace_back(path);
}

} // namespace

/** Find intersections from all the input paths. */
void pcg_build_intersection_contours(PCGContext& ctx) {
    auto& out = ctx.outputs();
    pcg_find_path_2d_intersections(ctx);

    RefPtr<PointArray> rawIntersections = out.get("Out");
    U32 nIntersecs = rawIntersections->get_num_points();
    PointArrayVector outPaths;

    for (U32 i = 0; i < nIntersecs; ++i) {
        auto iPoint = rawIntersections->get_point(i);
        auto itype = iPoint.get<I32>("intersect_type");

        switch (itype) {
        case ITYPE_4WAY:
            handle_intersection(ctx, iPoint, outPaths, true);
            break;
        case ITYPE_3WAY:
            handle_intersection(ctx, iPoint, outPaths, false);
            break;
        default:
            break;
        }
    }

    ctx.outputs().set("Out", outPaths, true);
}

} // namespace nv