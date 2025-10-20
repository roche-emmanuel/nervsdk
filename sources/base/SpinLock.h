#ifndef NV_SPINLOCK_
#define NV_SPINLOCK_

#include <atomic>
#include <mutex>
#include <thread>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/threading.h>
#endif

namespace nv {

// Original implementation:
// class SpinLock {
//     std::atomic_flag flag = ATOMIC_FLAG_INIT;

//   public:
//     void lock() {
//         while (flag.test_and_set(std::memory_order_acquire))
//             ;
//     }
//     bool try_lock() { return !flag.test_and_set(std::memory_order_acquire); }
//     void unlock() { flag.clear(std::memory_order_release); }
// };

// Updated implementation:
// cf. https://rigtorp.se/spinlock/
class SpinLock {

#ifdef __EMSCRIPTEN__
    std::atomic<I32> flag = {0};

  public:
    void lock() {
        while (flag.exchange(1, std::memory_order_acquire) != 0) {
            emscripten_thread_sleep(0);
        }
    }

    void unlock() { flag.store(0, std::memory_order_release); }

    bool try_lock() { return flag.exchange(1, std::memory_order_acquire) == 0; }
#else
    std::atomic<bool> lock_ = {false};

  public:
    void lock() noexcept {
        for (;;) {
            // Optimistically assume the lock is free on the first try
            if (!lock_.exchange(true, std::memory_order_acquire)) {
                return;
            }
            // Wait for lock to be released without generating cache misses
            while (lock_.load(std::memory_order_relaxed)) {
                // Issue X86 PAUSE or ARM YIELD instruction to reduce contention
                // between hyper-threads
#if defined(_MSC_VER)
                _mm_pause();
#else
                __builtin_ia32_pause();
#endif
            }
        }
    }

    auto try_lock() noexcept -> bool {
        // First do a relaxed load to check if lock is free in order to prevent
        // unnecessary cache misses if someone does while(!try_lock())
        return !lock_.load(std::memory_order_relaxed) &&
               !lock_.exchange(true, std::memory_order_acquire);
    }

    void unlock() noexcept { lock_.store(false, std::memory_order_release); }
#endif
};

using SpinLockGuard = std::lock_guard<SpinLock>;

#define WITH_SPINLOCK(sp) nv::SpinLockGuard lock(sp)            // NOLINT
#define WITH_MUTEXLOCK(sp) std::lock_guard<std::mutex> lock(sp) // NOLINT
#define WITH_RMUTEXLOCK(sp)                                                    \
    std::lock_guard<std::recursive_mutex> lock(sp)                // NOLINT
#define WITH_LOCK(m) std::lock_guard<decltype(m)> lock(m);        // NOLINT
#define WITH_UNIQUELOCK(m) std::unique_lock<decltype(m)> lock(m); // NOLINT

}; // namespace nv

#endif
