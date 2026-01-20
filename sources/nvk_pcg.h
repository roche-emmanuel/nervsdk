#ifndef NV_NVK_PCG_
#define NV_NVK_PCG_

#include <nvk_common.h>

#include <nvk/geometry/geometry2d.h>
#include <nvk/pcg/PCGContext.h>
#include <nvk/pcg/PCGGraph.h>
#include <nvk/pcg/Point.h>
#include <nvk/pcg/PointArray.h>
#include <nvk/pcg/PointAttribute.h>

namespace nv {

enum IntersectionType {
    ITYPE_NONE = 0,
    ITYPE_4WAY = 1,
    ITYPE_3WAY = 2,
};

void pcg_set_data_id(PCGContext& ctx);
void pcg_find_path_2d_intersections(PCGContext& ctx);
void pcg_build_intersection_contours(PCGContext& ctx);
void pcg_resample_paths(PCGContext& ctx);
void pcg_compute_path_offsets(PCGContext& ctx);

} // namespace nv

#endif
