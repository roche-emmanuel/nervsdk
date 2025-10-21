#ifndef NV_NVK_COMMON_
#define NV_NVK_COMMON_

#include <array>
#include <atomic>
#include <concepts>
#include <condition_variable>
#include <deque>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <list>
#include <map>
#include <mutex>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <windowsx.h>
#endif

#include <algorithm> // for std::min and std::max

#include <nvk_config.h>

#include <nvk_macros.h>
#include <nvk_math.h>
#include <nvk_memory.h>
#include <nvk_types.h>

#include <nvk/base/RefObject.h>
#include <nvk/base/RefPtr.h>
#include <nvk/base/std_containers.h>
#include <nvk/base/string_id.h>

#include <nvk/utils.h>

#include <nvk/log/LogManager.h>

#endif
