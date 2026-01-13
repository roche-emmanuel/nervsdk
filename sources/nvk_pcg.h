#ifndef NV_NVK_PCG_
#define NV_NVK_PCG_

#include <nvk_common.h>

#include <nvk/geometry/geometry2d.h>
#include <nvk/pcg/PCGContext.h>
#include <nvk/pcg/PCGGraph.h>
#include <nvk/pcg/PointArray.h>
#include <nvk/pcg/PointAttribute.h>

namespace nv {

void pcg_find_path_2d_intersections(PCGContext& ctx);

}

#endif
