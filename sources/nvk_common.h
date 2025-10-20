#ifndef NV_NVK_COMMON_
#define NV_NVK_COMMON_

#include <array>
#include <atomic>
#include <concepts>
#include <condition_variable>
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
#include <thread>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <windowsx.h>
#endif

#include <algorithm> // for std::min and std::max

#include <nvk_macros.h>
#include <nvk_math.h>
#include <nvk_types.h>

#endif
