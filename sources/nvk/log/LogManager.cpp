// Implementation for LogManager

#include <nvk/log/LogManager.h>
#include <nvk/log/StdLogger.h>
#include <nvk/utils.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

namespace nv {

#define NV_LOG_MSG_MAX_BULK_COUNT 1024

// Precomputed log level strings
constexpr const char* logLevelStrings[] = {"[FATAL] ", "[ERROR] ", "[WARN] ",
                                           "[NOTE] ",  "[INFO] ",  "[DEBUG] ",
                                           "[TRACE] ", "[???] "};
constexpr const int logLevelLens[] = {8, 8, 7, 7, 7, 8, 8, 6};

struct ThreadData {
    fmt::memory_buffer buffer;
    std::string str;
};

thread_local ThreadData threadData;

NV_IMPLEMENT_RAW_INSTANCE(LogManager)

LogManager::LogManager()
#if NV_USE_LOG_THREAD
    : _msgQueue(NV_LOG_MSG_QUEUE_CAPACITY),
      _msgConsumerToken(moodycamel::ConsumerToken(_msgQueue)),
      _recycleProducerToken(moodycamel::ProducerToken(_recycleQueue))
#endif
{};

LogManager::~LogManager() = default;

#if NV_USE_LOG_THREAD
void LogManager::logger_thread() {

    std::string buffer;

    U32 maxNumStrings = NV_LOG_MSG_MAX_BULK_COUNT;
    std::vector<MsgTag> mtags(maxNumStrings);

    U32 lastNumQueuedStrings = 0;
    U32 lastMaxCount = 0;

    while (true) {
        // if (!_msgQueue.wait_dequeue_timed(_msgConsumerToken, msg, 200)) {
        //     if (_stop) {
        //         break;
        //     }

        //     continue;
        // };

        // U32 count = _msgQueue.wait_dequeue_bulk_timed(
        //     _msgConsumerToken, mtags.data(), maxNumStrings, 200);
        U32 count = _msgQueue.wait_dequeue_bulk(_msgConsumerToken, mtags.data(),
                                                maxNumStrings);

        if (count == 0) {
            continue;
        };

        if (_stop) {
            _numPendingMessages.store(0, std::memory_order_release);
            break;
        }

        // We have count valid messages, which should be sorted by timetag:
        // Sort only the first 'count' objects by timetag
        std::sort(mtags.begin(), mtags.begin() + count,
                  [](const MsgTag& a, const MsgTag& b) {
                      return a.timetag < b.timetag;
                  });

        U32 num = _numQueuedStrings.load(std::memory_order_acquire);
        if (num != lastNumQueuedStrings) {
            lastNumQueuedStrings = num;
            // std::cout << "====> num queued strings: " << num << std::endl;
        }

        // Check if we have increased the count:
        if (count > lastMaxCount) {
            lastMaxCount = count;
            // std::cout << "====> Max dequeue bulk count: " << count <<
            // std::endl;
        }

        // Concatenate all the strings in a single large buffer:
        U32 tsize = 0;
        for (U32 i = 0; i < count; ++i) {
            U32 idx = mtags[i].index;
            tsize += _msgArray[idx].size();
        }

        // Add space for the newline character:
        tsize += count - 1;

        if (buffer.capacity() < tsize) {
            // std::cout << "====> Increasing buffer capacity to: " << tsize
            // << std::endl;
            buffer.reserve(tsize);
        }

        // Copy the data:
        // char* ptr = buffer.data();
        buffer.clear();
        for (U32 i = 0; i < count; ++i) {
            U32 idx = mtags[i].index;
            buffer += _msgArray[idx];
            if (i < (count - 1)) {
                // Add the newline:
                buffer += '\n';
            }
        }

        // recycle the strings:
        _recycleQueue.enqueue_bulk(_recycleProducerToken, mtags.data(), count);

        // We have a string to output:
        output_message(0, buffer);

        // Update the count of pending messages:
        _numPendingMessages.fetch_sub(count, std::memory_order_release);

        // // Recycle the message string:
        // _recycleQueue.enqueue(_recycleProducerToken, msg);
    }

    // std::cout << "====> Exiting logger thread." << std::endl;
}
#endif

void LogManager::init_instance() {
#if NV_USE_LOG_THREAD
    _logThread =
        std::make_unique<std::thread>(&LogManager::logger_thread, this);
#endif
}

void LogManager::uninit_instance() {
#if NV_USE_LOG_THREAD
    while (!is_idle()) {
        sleep_ms(10);
    }

    _stop = true;
    // Post a dummy message:
    MsgTag mtag{};
    while (!_msgQueue.enqueue(mtag))
        ;
    _logThread->join();
    _logThread.reset();
#endif
    _sinks.clear();
}

auto LogManager::get_output_string() -> std::string& {
#if 1
    return threadData.str;
#else
    auto tid = std::this_thread::get_id();
    auto it = _outStrings.find(tid);
    if (it != _outStrings.end()) {
        return it->second;
    }

    // We must add the mem buffer:
    std::string* bufPtr = nullptr;
    {
        WITH_SPINLOCK(_logSP);
        bufPtr = &_outStrings[tid];
    }

    return *bufPtr;
#endif
}

auto LogManager::get_mem_buffer() -> fmt::memory_buffer& {
#if 1
    return threadData.buffer;
#else
    auto tid = std::this_thread::get_id();
    auto it = _memBuffers.find(tid);
    if (it != _memBuffers.end()) {
        return it->second;
    }

    // We must add the mem buffer:
    fmt::memory_buffer* bufPtr = nullptr;
    {
        WITH_SPINLOCK(_logSP);
        bufPtr = &_memBuffers[tid];
    }

    // logNOTE("Adding logmanager mem buffer for thread {}",
    //         get_current_thread_id());

    return *bufPtr;
#endif
}

void LogManager::do_log(U32 lvl, const char* data, size_t size) {

    // Try to send that message to the logger thread:
#if NV_USE_LOG_THREAD
    _numPendingMessages.fetch_add(1, std::memory_order_release);
#endif

    std::array<char, 40> buf = {0};

#ifdef __EMSCRIPTEN__
    // Emscripten-specific high precision timing
    // Get standard timestamp for date/time components
    using namespace std::chrono;
    system_clock::time_point tp = system_clock::now();
    auto timeT = system_clock::to_time_t(tp);
    std::tm* tm = std::localtime(&timeT);

    // Format the date and seconds portion
    std::strftime(buf.data(), buf.size(), "%Y-%m-%d %H:%M:%S", tm);

    // Get high precision time using Emscripten's native function
    double now = emscripten_get_now();

    // Extract just the fractional part within the current second
    double intPart;
    double fractionalSeconds = std::modf(now / 1000.0, &intPart);

    // Convert to microseconds (6 decimal places) to match the original format
    int microsecs = static_cast<int>(fractionalSeconds * 1000000);

    // Append the fractional part with microsecond precision
    sprintf((char*)&buf[19], ".%06d ", microsecs);
#else
    // Original timing code for non-Emscripten platforms
    using namespace std::chrono;
    system_clock::time_point tp = system_clock::now();
    auto timePoint = time_point_cast<seconds>(tp); // Truncate to seconds
    auto fractionalSeconds = tp - timePoint;
    // Convert fractional seconds to microseconds
    int microsecs = (int)duration_cast<microseconds>(fractionalSeconds).count();
    auto timeT = system_clock::to_time_t(tp);
    std::tm* tm = std::localtime(&timeT);
    // Format time
    std::strftime(buf.data(), buf.size(), "%Y-%m-%d %H:%M:%S", tm);
    sprintf((char*)&buf[19], ".%06d ", microsecs);
#endif

    lvl = minimum(lvl, 7U);
    memcpy((char*)&buf[27], logLevelStrings[lvl], logLevelLens[lvl]);

#if NV_USE_LOG_THREAD
    // If using the log thread we first try to recycle the memory from a
    // previously used string:
    MsgTag mtag{};

    if (!_recycleQueue.try_dequeue_from_producer(_recycleProducerToken, mtag)) {
        // There is no MsgTag ready for dequeueing yet, so we check if we are
        // already at full capacity, or if we can start using a remaining unused
        // slot:
        if (_numQueuedStrings.load() == NV_LOG_MSG_QUEUE_CAPACITY) {
            // All slots are used so we must really wait until one becomes free:
            std::cout
                << "LogManager: =========> All string slots in use, waiting."
                << std::endl;
            while (!_recycleQueue.try_dequeue_from_producer(
                _recycleProducerToken, mtag))
                ;

        } else {
            // We can start using an unused slot increasing the count of "in
            // flight strings"
            mtag.index = _numQueuedStrings.fetch_add(1);
            // std::cout << "==========> Num queued strings: "
            //           << _numQueuedStrings.load(std::memory_order_acquire)
            //           << std::endl;
            // std::cout.flush();
        }
    }

    auto& str = _msgArray[mtag.index];

    // else {
    //     std::cout << "==========> Dequeuing string of capacity: "
    //               << str.capacity() << std::endl;
    //     std::cout.flush();
    // };

    // Note: whether the dequeue operation was successfull or not should not be
    // relevant here: if there is nothing to dequeue, we just use the string as
    // is (eg. allocating some additional memory for it)
    U32 tsize = strlen(buf.data()) + size;
    if (str.capacity() < tsize) {
        // std::cout << "==========> Increasing string capacity to " << tsize
        //           << std::endl;
        str.reserve(tsize);
    }
    // else {
    //     std::cout << "==========> Reusing string with capacity "
    //               << str.capacity() << std::endl;
    // }

    str.clear();
    str += buf.data();
    str.append(data, size);

    // Update the timetag:
    mtag.timetag = _timeTag.fetch_add(1);

    while (!_msgQueue.enqueue(mtag))
        ;

#else
    auto& str = get_output_string();
    str.reserve(strlen(buf.data()) + size);
    str.clear();
    str += buf.data();
    str.append(data, size);

    // lock the logging spinlock:
    // Note: on emscripten it seems preferable to use a regular mutex instead of
    // a spinlock. WITH_SPINLOCK(_logSP);
    WITH_MUTEXLOCK(_logMutex);
    output_message(lvl, str);
#endif

    // std::cout.write(data, (std::streamsize)size) << std::endl;
}

void LogManager::output_message(U32 lvl, const std::string& msg) {

    if (_sinks.empty()) {
        add_sink(new StdLogger());
    }

    for (auto& sink : _sinks) {
        // sink->output(lvl, buf.data(), data, size);
        sink->output((I32)lvl, nullptr, msg.data(), msg.size());
    }
}

auto LogManager::remove_sink(LogSink* sink) -> bool {
    NVCHK(sink != nullptr, "Invalid log sink");
    return remove_vector_element(_sinks, sink);
}

} // namespace nv
