// precomp.h auto included:
// #include <nvk_precomp.h>

// Note we cannot include this here as this will conflict with Unreal engine
// libraries:

// #define STB_IMAGE_IMPLEMENTATION #include
// <external/stb/stb_image.h>

// #define STB_IMAGE_RESIZE_IMPLEMENTATION
// #include <external/stb/stb_image_resize.h>

// #define STB_IMAGE_WRITE_IMPLEMENTATION
// #include <external/stb/stb_image_write.h>

// On emscripten we only have support for 32bit target for now.
#ifndef __EMSCRIPTEN__
static_assert(sizeof(size_t) == 8, "Invalid size_t type");
#endif
