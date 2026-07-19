#ifndef NV_NVK

#ifdef NV_TRACY_ENABLE
#include <tracy/Tracy.hpp>
#define TRACY_ZONE() ZoneScoped
#define TRACY_ZONE_N(name) ZoneScopedN(name)
#else
#define TRACY_ZONE()                                                           \
    do {                                                                       \
    } while (0)
#define TRACY_ZONE_N(name)                                                     \
    do {                                                                       \
    } while (0)
#endif

#include <nvk_common.h>

#include <nvk/utils.h>

#endif
