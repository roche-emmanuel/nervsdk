#ifndef _NV_STOPWATCH_H_
#define _NV_STOPWATCH_H_

#include <nvk_common.h>

#include <algorithm>
#include <chrono>
#include <cmath>

namespace nv {

/// Wall clock stopwatch (steady clock), started when constructed.
class Stopwatch {
    using Clock = std::chrono::steady_clock;

  public:
    Stopwatch() : _start(Clock::now()) {}

    /// Seconds since construction or the last restart().
    [[nodiscard]] auto seconds() const -> F64 {
        return std::chrono::duration<F64>(Clock::now() - _start).count();
    }

    void restart() { _start = Clock::now(); }

  private:
    Clock::time_point _start;
};

/// Adds the time spent in a scope to an accumulator (seconds).
class ScopeAccumTimer {
  public:
    explicit ScopeAccumTimer(F64& acc) : _acc(acc) {}
    ~ScopeAccumTimer() { _acc += _timer.seconds(); }
    ScopeAccumTimer(const ScopeAccumTimer&) = delete;
    auto operator=(const ScopeAccumTimer&) -> ScopeAccumTimer& = delete;

  private:
    F64& _acc;
    Stopwatch _timer;
};

/// Short duration for log lines: "850 ms", "12.3 s", "4m07s", "2h03m10s".
/// "?" for a negative or non finite input. (format_duration() is the long
/// canonical form.)
[[nodiscard]] inline auto format_duration_compact(F64 sec) -> String {
    if (!std::isfinite(sec) || sec < 0.0) {
        return "?";
    }
    if (sec < 1.0) {
        return fmt::format("{:.0f} ms", sec * 1000.0);
    }
    if (sec < 60.0) {
        return fmt::format("{:.1f} s", sec);
    }
    const I64 total = I64(sec + 0.5);
    if (total < 3600) {
        return fmt::format("{}m{:02}s", total / 60, total % 60);
    }
    return fmt::format("{}h{:02}m{:02}s", total / 3600, (total / 60) % 60,
                       total % 60);
}

/**
Progress line for a long loop: at most one line every `intervalSec`, with the
elapsed time and an ETA extrapolated from the rate so far (so only indicative
when the cost of the items is uneven). Weigh the items (points, ribs...) so
that `done / total` follows the work, not the item count.
*/
class ProgressLogger {
  public:
    ProgressLogger(String label, U64 total, F64 intervalSec = 5.0)
        : _label(std::move(label)), _total(total), _interval(intervalSec) {}

    void update(U64 done) {
        if (_total == 0) {
            return;
        }
        const F64 now = _timer.seconds();
        if (now - _lastLog < _interval) {
            return;
        }
        _lastLog = now;
        const F64 ratio = std::min(1.0, F64(done) / F64(_total));
        const F64 eta = ratio > 1e-3 ? now * (1.0 - ratio) / ratio : -1.0;
        logINFO("  {}: {}/{} ({:.1f}%), elapsed {}, ETA {}", _label, done,
                _total, ratio * 100.0, format_duration_compact(now),
                eta >= 0.0 ? format_duration_compact(eta) : String("?"));
    }

    [[nodiscard]] auto elapsed() const -> F64 { return _timer.seconds(); }

  private:
    String _label;
    U64 _total;
    F64 _interval;
    F64 _lastLog{0.0};
    Stopwatch _timer;
};

} // namespace nv

#endif
