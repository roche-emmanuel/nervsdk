#ifndef NV_NVK

#ifdef NV_TRACY_ENABLE
#include <tracy/Tracy.hpp>
#define TRACY_ZONE() ZoneScoped
#define TRACY_ZONE_N(name) ZoneScopedN(name)
#define TRACY_FRAME_MARK() FrameMark
#define TRACY_ZONE_TEXT(txt, len) ZoneText(txt, len)
#define TRACY_PLOT(name, value) TracyPlot(name, (double)(value))
#define TRACY_MESSAGE(txt) TracyMessageL(txt)
#else
#define TRACY_ZONE()                                                           \
    do {                                                                       \
    } while (0)
#define TRACY_ZONE_N(name)                                                     \
    do {                                                                       \
    } while (0)
#define TRACY_FRAME_MARK()                                                     \
    do {                                                                       \
    } while (0)
#define TRACY_ZONE_TEXT(txt, len)                                              \
    do {                                                                       \
    } while (0)
#define TRACY_PLOT(name, value)                                                \
    do {                                                                       \
    } while (0)
#define TRACY_MESSAGE(txt)                                                     \
    do {                                                                       \
    } while (0)
#endif

#include <nvk_common.h>

#include <nvk/utils.h>

#endif
