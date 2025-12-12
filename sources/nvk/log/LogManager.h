#ifndef NV_LOGMANAGER_
#define NV_LOGMANAGER_

#include <nvk_common.h>

#include <external/blockingconcurrentqueue.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include <nvk/base/RefObject.h>
#include <nvk/base/RefPtr.h>
#include <nvk/base/SpinLock.h>
#include <nvk/base/std_containers.h>
#include <nvk/log/LogSink.h>

#define NV_LOG_MSG_QUEUE_CAPACITY 1024

namespace nv {

template <typename T> struct is_vector : std::false_type {};

template <typename U> struct is_vector<Vector<U>> : std::true_type {};

template <typename T> inline constexpr bool is_vector_v = is_vector<T>::value;

class LogManager {
    NV_DECLARE_RAW_INSTANCE(LogManager)

  public:
    virtual ~LogManager();

    using RedirectFunc = void (*)(U32, const char*, size_t);

    enum Level : int {
        L_FATAL,
        L_ERROR,
        L_WARN,
        L_NOTE,
        L_INFO,
        L_DEBUG,
        L_TRACE,
    };

    void set_notify_level(Level lvl) { _notifyLevel = lvl; }

    void log_message(int lvl, const char* data) {
        if (lvl > _notifyLevel) {
            return; // Discarding.
        }

        do_log(lvl, data, strlen(data));
    }

    template <typename... Args>
    void log(int lvl, fmt::format_string<Args...> fmt_str, Args&&... args) {
        if (lvl > _notifyLevel) {
            return; // Discarding.
        }
        // std::cout << "Formatting with: " << fmt_str << std::endl;

        // size_t size =
        //     snprintf(nullptr, 0, fmt_str, std::forward<Args>(args)...) +
        //     1; // Extra space for '\0'
        // std::unique_ptr<char[]> buf(new char[size]);
        // snprintf(buf.get(), size, fmt_str, std::forward<Args>(args)...);
        // // return String(buf.get(), buf.get() + size - 1); // We don't want
        // the
        // // '\0' inside
        // std::cout.write(buf.get(), (std::streamsize)size - 1) << std::endl;

        // std::string out = fmt::format(fmt_str, std::forward<Args>(args)...);
        // auto out = fmt::memory_buffer();

        auto& buf = get_mem_buffer();
        buf.clear();
        fmt::format_to(std::back_inserter(buf), fmt_str,
                       std::forward<Args>(args)...);
        auto* data = buf.data(); // pointer to the formatted data
        auto size = buf.size();  // size of the formatted data
        do_log(lvl, data, size);
    }

    static void debug(const char* msg) {
        LogManager::instance().log_message(L_DEBUG, msg);
    }

    static void fatal(const char* msg) {
        LogManager::instance().log_message(L_FATAL, msg);
    }

    template <typename... Args>
    static void trace(fmt::format_string<Args...> fmt, Args&&... args) {
        LogManager::instance().log(L_TRACE, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void debug(fmt::format_string<Args...> fmt, Args&&... args) {
        LogManager::instance().log(L_DEBUG, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void debug_1s(StringID logId, fmt::format_string<Args...> fmt,
                         Args&&... args) {
        auto& lman = LogManager::instance();
        if (lman.should_log(logId, std::chrono::seconds(1))) {
            LogManager::instance().log(L_DEBUG, fmt,
                                       std::forward<Args>(args)...);
        }
    }

    template <typename... Args>
    static void info(fmt::format_string<Args...> fmt, Args&&... args) {
        LogManager::instance().log(L_INFO, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void note(fmt::format_string<Args...> fmt, Args&&... args) {
        LogManager::instance().log(L_NOTE, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void warn(fmt::format_string<Args...> fmt, Args&&... args) {
        LogManager::instance().log(L_WARN, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void error(fmt::format_string<Args...> fmt, Args&&... args) {
        LogManager::instance().log(L_ERROR, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void fatal(fmt::format_string<Args...> fmt, Args&&... args) {
        LogManager::instance().log(L_FATAL, fmt, std::forward<Args>(args)...);
    }

    void add_sink(LogSink* sink) { _sinks.emplace_back(sink); }

    /** Remove a log sink */
    auto remove_sink(LogSink* sink) -> bool;

    auto is_idle() const -> bool {
#if NV_USE_LOG_THREAD
        return _numPendingMessages.load(std::memory_order_acquire) == 0;
#else
        // LogManager is always considered idle:
        return true;
#endif
    }

    // Assign redirect function:
    void set_redirect_func(RedirectFunc func);

  protected:
    struct MsgTag {
        U32 index{0};
        U64 timetag{0};
    };

    void do_log(U32 lvl, const char* data, size_t size);

    auto get_mem_buffer() -> fmt::memory_buffer&;

    auto get_output_string() -> std::string&;

    /** Output a message */
    void output_message(U32 lvl, const std::string& msg);

    void clear_buffer() { get_mem_buffer().clear(); };

#if 0
    /** We should have one buffer per thread */
    std::unordered_map<std::thread::id, fmt::memory_buffer> _memBuffers;

    /** We should have one output string per thread */
    std::unordered_map<std::thread::id, std::string> _outStrings;
#endif

    // fmt::memory_buffer _buf;

  private:
    int _notifyLevel = L_INFO;

    RedirectFunc _redirectFn{nullptr};

    SpinLock _logSP;
    std::mutex _logMutex;

    /** Note: Cannot use an nv::Vector container here
    or we will get a memory lead on close: */
    std::vector<RefPtr<LogSink>> _sinks;

    /** throtlling data */
    std::unordered_map<StringID, std::chrono::steady_clock::time_point>
        _lastLogTimeMap;

    auto should_log(StringID logId, std::chrono::milliseconds throttle_period =
                                        std::chrono::seconds(1)) -> bool {
        auto now = std::chrono::steady_clock::now();
        auto it = _lastLogTimeMap.find(logId);

        if (it == _lastLogTimeMap.end()) {
            _lastLogTimeMap.insert(std::make_pair(logId, now));
            return true;
        }

        if ((now - it->second) >= throttle_period) {
            it->second = now;
            return true;
        }

        return false;
    }

#if NV_USE_LOG_THREAD
    /** The thread object */
    std::unique_ptr<std::thread> _logThread;

    /** Stop flag */
    Bool _stop{false};

    /** log thread entrypoint */
    void logger_thread();

    /** Count of pending log messages */
    std::atomic<U32> _numQueuedStrings;

    /** Number of pending messages */
    std::atomic<U32> _numPendingMessages{0};

    /** time tag for the messages */
    std::atomic<U64> _timeTag;

    /** Message queue */
    using MessageQueue = moodycamel::BlockingConcurrentQueue<MsgTag>;
    MessageQueue _msgQueue;

    /** Additional queue to recycle the memory allocated for the output strings
     */
    using RecycleQueue = moodycamel::ConcurrentQueue<MsgTag>;
    RecycleQueue _recycleQueue;

    /** Logger thread consummer token */
    moodycamel::ConsumerToken _msgConsumerToken;

    /** Logger thread producer token */
    moodycamel::ProducerToken _recycleProducerToken;

    /** Storage for the messages */
    std::array<std::string, NV_LOG_MSG_QUEUE_CAPACITY> _msgArray;

#endif
};

template <typename... Args>
inline auto format_msg(fmt::format_string<Args...> msg_format, Args&&... args)
    -> String {
    auto out = fmt::memory_buffer();
    fmt::format_to(std::back_inserter(out), msg_format,
                   std::forward<Args>(args)...);
    // auto* data = out.data(); // pointer to the formatted data
    return {out.data(), out.size()};
}

template <typename... Args>
inline void throw_msg(fmt::format_string<Args...> msg_format, Args&&... args) {
    // logFATAL(fmt, std::forward<Args>(args)...);
    auto out = fmt::memory_buffer();
    fmt::format_to(std::back_inserter(out), msg_format,
                   std::forward<Args>(args)...);
    auto* data = out.data(); // pointer to the formatted data
    std::string str(data, out.size());
    std::cout << "[FATAL Error]: " << str << std::endl;
    logFATAL(str.c_str());
    // Wait until the LogManager is done.
    auto& lman = LogManager::instance();
    I32 count = 0;
    while (!lman.is_idle()) {
        if ((++count % 100) == 0) {
            std::cout << "Waiting for LogManager to become idle..."
                      << std::endl;
        }
        sleep_ms(10);
    }

    // auto size = out.size();  // size of the formatted data
    // nv::String msg(data, size);
    // THROW(std::runtime_error(msg.c_str()));
#ifdef __cpp_exceptions
    throw std::runtime_error(str.c_str());
#else
    logFATAL(
        "Terminating program (Cannot throw when exceptions are disabled).");
    std::terminate();
#endif
}

template <typename... Args>
inline void check_cond(bool cond, fmt::format_string<Args...> fmt,
                       Args&&... args) {
    if (!cond) {
        throw_msg(fmt, std::forward<Args>(args)...);
    }
}

template <typename... Args>
inline void check_no_throw(bool cond, fmt::format_string<Args...> fmt,
                           Args&&... args) {
    if (!cond) {
        nv::LogManager::fatal(fmt, std::forward<Args>(args)...);
    }
}

} // namespace nv

template <typename T> struct fmt::formatter<nv::Vector<T>> {
    // Parsing the format specifier (if any)
    constexpr auto parse(format_parse_context& ctx) const
        -> decltype(ctx.begin()) {
        const auto* it = ctx.begin();
        const auto* end = ctx.end();

        // Check if reached the end of the range:
        if (it != end && *it != '}') {
            throw format_error("invalid format");
        }

        // Return an iterator past the end of the parsed range:
        return it;
    }

    // Overloaded format method
    template <typename FormatContext>
    auto format(const nv::Vector<T>& vec, FormatContext& ctx) const
        -> decltype(ctx.out()) {
        auto out = ctx.out();
        // Opening bracket for the vector
        out = fmt::format_to(out, "[");

        // Formatting each element in the vector
        for (size_t i = 0; i < vec.size(); ++i) {
            // Use the default formatter for type T
            if (i > 0) {
                out = fmt::format_to(out, ", ");
            }
            out = fmt::format_to(out, "{}", vec[i]);
        }

        // Closing bracket for the vector
        return fmt::format_to(out, "]");
    }
};

template <typename T> struct fmt::formatter<nv::Set<T>> {
    // Parsing the format specifier (if any)
    constexpr auto parse(format_parse_context& ctx) const
        -> decltype(ctx.begin()) {
        const auto* it = ctx.begin();
        const auto* end = ctx.end();

        // Check if reached the end of the range:
        if (it != end && *it != '}') {
            throw format_error("invalid format");
        }

        // Return an iterator past the end of the parsed range:
        return it;
    }

    // Overloaded format method
    template <typename FormatContext>
    auto format(const nv::Set<T>& vec, FormatContext& ctx) const
        -> decltype(ctx.out()) {
        auto out = ctx.out();
        // Opening bracket for the vector
        out = fmt::format_to(out, "{{");

        // Formatting each element in the vector

        for (auto it = vec.begin(); it != vec.end(); ++it) {
            // Use the default formatter for type T
            if (it != vec.begin()) {
                out = fmt::format_to(out, ", ");
            }
            out = fmt::format_to(out, "{}", *it);
        }

        // Closing bracket for the vector
        return fmt::format_to(out, "}}");
    }
};

// Generic formatter for all enums - converts to underlying type
template <typename T>
struct fmt::formatter<T, std::enable_if_t<std::is_enum_v<T>, char>>
    : fmt::formatter<std::underlying_type_t<T>> {

    auto format(const T& value, format_context& ctx) const {
        return fmt::formatter<std::underlying_type_t<T>>::format(
            static_cast<std::underlying_type_t<T>>(value), ctx);
    }
};

#endif
