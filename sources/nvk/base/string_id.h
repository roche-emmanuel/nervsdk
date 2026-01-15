#ifndef _STRING_ID_H_
#define _STRING_ID_H_

#include <nvk_types.h>

namespace nv {

inline auto str_id(const String& str) noexcept -> StringID {
    return hash_64_fnv1a(str.data(), str.size());
}

} // namespace nv

#endif
