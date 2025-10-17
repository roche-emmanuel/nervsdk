// precomp.h auto included:
// #include <nvk_precomp.h>

// On emscripten we only have support for 32bit target for now.
#ifndef __EMSCRIPTEN__
static_assert(sizeof(size_t) == 8, "Invalid size_t type");
#endif
