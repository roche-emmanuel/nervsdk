#ifndef NV_NVK_BASE_
#define NV_NVK_BASE_

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
#include <queue>
#include <random>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <windowsx.h>
#endif

#include <algorithm> // for std::min and std::max

#include <fmt/core.h>
#include <fmt/format.h>

#include <cmath>

#include <external/json.hpp>

namespace nv {
using Json = nlohmann::json;
using OrderedJson = nlohmann::ordered_json;

class PointArray;
class PCGPoint;
class PCGPointRef;

} // namespace nv

#endif
