#include <nvk/task/Promise.h>

namespace nv {

// Defer implementation
void Defer::resolve() const {
    if (_promise) {
        _promise->resolve_internal();
    }
}

void Defer::resolve(const Any& value) const {
    if (_promise) {
        _promise->resolve_internal(value);
    }
}

void Defer::reject() const {
    if (_promise) {
        _promise->reject_internal();
    }
}

void Defer::reject(const Any& error) const {
    if (_promise) {
        _promise->reject_internal(error);
    }
}

// PromiseBase implementation
PromiseBase::PromiseBase() = default;

void PromiseBase::resolve_internal() {
    if (_settling.exchange(true, std::memory_order_acq_rel)) {
        return;
    }
    _state.store(PromiseState::RESOLVED, std::memory_order_release);
    execute_continuations();
}

void PromiseBase::resolve_internal(const Any& value) {
    if (_settling.exchange(true, std::memory_order_acq_rel)) {
        return;
    }
    _value = value;
    _state.store(PromiseState::RESOLVED, std::memory_order_release);
    execute_continuations();
}

void PromiseBase::reject_internal() {
    if (_settling.exchange(true, std::memory_order_acq_rel)) {
        return;
    }
    _state.store(PromiseState::REJECTED, std::memory_order_release);
    execute_continuations();
}

void PromiseBase::reject_internal(const Any& error) {
    if (_settling.exchange(true, std::memory_order_acq_rel)) {
        return;
    }
    _error = error;
    _state.store(PromiseState::REJECTED, std::memory_order_release);
    execute_continuations();
}

void PromiseBase::execute_continuations() {
    Vector<Continuation> continuations;

    {
        std::lock_guard lock(_mutex);
        if (_continuations.empty()) {
            return;
        }

        continuations = std::move(_continuations);
        _continuations.clear();
    }

    auto& dispatch = JobDispatcher::instance();
    bool isMain = dispatch.is_main_thread();

    for (auto& cont : continuations) {
        if (isMain && cont.onMain) {
            cont.job();
        } else {
            dispatch.post(std::move(cont.job), cont.onMain);
        }
    }
}

void PromiseBase::add_continuation(Continuation continuation) {
    if (is_pending()) {
        std::lock_guard lock(_mutex);
        // Re-check under the lock to avoid a race with settlement.
        if (is_pending()) {
            _continuations.push_back(std::move(continuation));
            return;
        }
    }
    auto& dispatch = JobDispatcher::instance();
    if (dispatch.is_main_thread() && continuation.onMain) {
        // Don't schedule this task, instead run it here immediately:
        continuation.job();
    } else {
        dispatch.post(std::move(continuation.job), continuation.onMain);
    }
}

} // namespace nv