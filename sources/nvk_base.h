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
#include <random>
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

#include <fmt/core.h>
#include <fmt/format.h>

#include <cmath>

#endif
