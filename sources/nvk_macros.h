#ifndef NV_NVK_MACROS_
#define NV_NVK_MACROS_

#include <cassert>
#include <sstream>

namespace nv {
class LogManager;
}

#define logTRACE nv::LogManager::trace
#define logDEBUG nv::LogManager::debug
#define logDEBUG_1S nv::LogManager::debug_1s
#define logINFO nv::LogManager::info
#define logNOTE nv::LogManager::note
#define logWARN nv::LogManager::warn
#define logERROR nv::LogManager::error
#define logFATAL nv::LogManager::fatal
#define NVCHK nv::check
#define CHECK_NO_THROW nv::check_no_throw
#define THROW_MSG nv::throw_msg

#ifdef assert
#undef assert
#endif

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define VALIDATE_RET(exp)                                                      \
    auto __res = (exp);                                                        \
    if (!__res) {                                                              \
        THROW_MSG("Invalid result for {}", #exp);                              \
    }                                                                          \
    return __res;

#define NV_DEPRECATED(msg)                                                     \
    {                                                                          \
        logWARN("[DEPRECATED] (" << __FILE__ << ":" << __LINE__ << ") "        \
                                 << msg);                                      \
    }

#define NO_IMPL(msg)                                                           \
    {                                                                          \
        auto msg_str__ = format_msg(msg);                                      \
        THROW_MSG("[NO_IMPL] ({}:{}) {}", __FILE__, __LINE__,                  \
                  msg_str__.c_str());                                          \
    }

#ifndef NV_PRODUCTION_MODE
#define assert_error_report_helper(cond) "assertion failed: " #cond
#define NV_ASSERT(cond)                                                        \
    {                                                                          \
        if (!(cond)) {                                                         \
            THROW_MSG(assert_error_report_helper(cond));                       \
        }                                                                      \
    }
#define NV_SOFT_ASSERT(cond)                                                   \
    {                                                                          \
        if (!(cond)) {                                                         \
            logFATAL(assert_error_report_helper(cond));                        \
            std::terminate();                                                  \
        }                                                                      \
    }
#else
#define NV_ASSERT(cond)
#define NV_SOFT_ASSERT(cond)
#endif

#define NV_DECLARE_NO_COPY(cname)                                              \
  public:                                                                      \
    cname(const cname&) = delete;                                              \
    auto operator=(const cname&)->cname& = delete;

#define NV_DECLARE_DEFAULT_COPY(cname)                                         \
  public:                                                                      \
    cname(const cname&) = default;                                             \
    auto operator=(const cname&)->cname& = default;

#define NV_DECLARE_NO_MOVE(cname)                                              \
  public:                                                                      \
    cname(cname&&) = delete;                                                   \
    auto operator=(cname&&)->cname& = delete;

#define NV_DECLARE_DEFAULT_MOVE(cname)                                         \
  public:                                                                      \
    cname(cname&&) noexcept = default;                                         \
    auto operator=(cname&&) noexcept -> cname& = default;

#endif
