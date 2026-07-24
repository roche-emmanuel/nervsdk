#include <nvk/geometry/geometry2d.h>

namespace nv {

auto samples_apply_normal_offset(const ProfileVec2d& cline, F64 offset)
    -> ProfileVec2d {
    ProfileVec2d res;
    U32 num = cline.size();
    NVCHK(num >= 2, "Invalid number of points.");
    res.samples.reserve(num);
    Vec2d n;
    const auto& samples = cline.samples;
    for (I32 i = 0; i < num; ++i) {
        if (i < (num - 1)) {
            n = (samples[i + 1].v - samples[i].v).normalized().ccw90();
        }
        res.samples.emplace_back(
            SampleVec2d{.t = samples[i].t, .v = samples[i].v + n * offset});
    }
    return res;
};

} // namespace nv