#pragma once
#ifndef NV_CANCELLATION_H_
#define NV_CANCELLATION_H_

#include <nvk_base.h>

namespace nv {

// ---------------------------------------------------------------------------
// CancelledError  –  sentinel type stored in Any when a promise is cancelled.
// Catch-error handlers can test for this to distinguish cancellation from
// real failures:
//
//   .catch_error([](const Any& e) {
//       if (e.is<CancelledError>()) return; // swallow
//       throw;                               // re-raise real errors
//   });
// ---------------------------------------------------------------------------
struct CancelledError {
    [[nodiscard]] auto what() const noexcept -> const char* {
        return "Promise was cancelled";
    }
};

NV_DEFINE_TYPE_ID(nv::CancelledError);

// ---------------------------------------------------------------------------
// Internal shared state between CancellationSource and CancellationToken.
// ---------------------------------------------------------------------------
struct CancellationState {
    std::atomic<bool> cancelled{false};

    void cancel() noexcept { cancelled.store(true, std::memory_order_release); }

    auto is_cancelled() const noexcept -> bool {
        return cancelled.load(std::memory_order_acquire);
    }
};

// ---------------------------------------------------------------------------
// CancellationToken  –  read-only handle passed to promises / continuations.
// Cheap to copy (shared_ptr under the hood).
// ---------------------------------------------------------------------------
class CancellationToken {
  public:
    // A default-constructed token is never cancelled and never will be.
    CancellationToken() : _state(std::make_shared<CancellationState>()) {}

    explicit CancellationToken(std::shared_ptr<CancellationState> state)
        : _state(std::move(state)) {}

    [[nodiscard]] auto is_cancelled() const noexcept -> bool {
        return _state && _state->is_cancelled();
    }

    // Throws CancelledError if cancelled.  Convenient inside executor lambdas:
    //   token.throw_if_cancelled();
    void throw_if_cancelled() const {
        if (is_cancelled())
            throw CancelledError{};
    }

    // Sentinel "never cancelled" token – use when no cancellation is needed.
    static CancellationToken none() { return CancellationToken{}; }

  private:
    std::shared_ptr<CancellationState> _state;

    friend class CancellationSource;
};

// ---------------------------------------------------------------------------
// CancellationSource  –  the owning side.  One source, many tokens.
//
//   CancellationSource src;
//   CancellationToken  tok = src.token();
//
//   auto p = make_promise([](nv::Defer d) { ... }, tok);
//
//   src.cancel();   // rejects p (and any chained promises) with CancelledError
// ---------------------------------------------------------------------------
class CancellationSource {
  public:
    CancellationSource() : _state(std::make_shared<CancellationState>()) {}

    // Non-copyable – there should be one clear owner.
    CancellationSource(const CancellationSource&) = delete;
    auto operator=(const CancellationSource&) -> CancellationSource& = delete;

    CancellationSource(CancellationSource&&) = default;
    auto operator=(CancellationSource&&) -> CancellationSource& = default;

    // Signals cancellation on all tokens derived from this source.
    void cancel() noexcept { _state->cancel(); }

    [[nodiscard]] auto is_cancelled() const noexcept -> bool {
        return _state->is_cancelled();
    }

    // Returns a lightweight token that can be freely copied and passed around.
    [[nodiscard]] auto token() const -> CancellationToken {
        return CancellationToken{_state};
    }

  private:
    std::shared_ptr<CancellationState> _state;
};

} // namespace nv

#endif // NV_CANCELLATION_H_