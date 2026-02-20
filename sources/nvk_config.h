#ifndef NV_NVK_CONFIG_
#define NV_NVK_CONFIG_

#define NV_MAX_OBJECT_NAME_LEN 32

#define NV_MAX_NUM_THREADS 16

#ifdef __EMSCRIPTEN__
#define NV_USE_LOG_THREAD 1
// This  doesn't seem to work correctly on emscripten (?)
#define NV_CHECK_MEMORY_LEAKS 0
#else
#define NV_USE_LOG_THREAD 1
#define NV_CHECK_MEMORY_LEAKS 1
#endif

// Note: it seems that the emscripten compiler doesn't like my custom memory
// manager layer very much.
#define NV_USE_STD_MEMORY 1

namespace nv {

constexpr double SPHERICAL_EARTH_RADIUS = 6360000.0;
constexpr double MEAN_EARTH_RADIUS = 6371000.0;

} // namespace nv

#endif
