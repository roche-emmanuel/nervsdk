#ifndef NV_PROMISE_
#define NV_PROMISE_

#include <nvk_base.h>

#include <nvk/base/WeakPtr.h>
#include <nvk/base/WeakRefObject.h>
#include <nvk/task/Cancellation.h>
#include <nvk/task/JobDispatcher.h>

namespace nv {

class PromiseBase;
template <typename T> class Promise;

// ---------------------------------------------------------------------------
// Callable signature introspection helpers
// ---------------------------------------------------------------------------
namespace detail {

template <typename F, typename = void> struct callable_traits;

template <typename R, typename... Args> struct callable_traits<R (*)(Args...)> {
    using return_type = R;
    using arg_types = std::tuple<Args...>;
    static constexpr std::size_t arity = sizeof...(Args);
};

template <typename C, typename R, typename... Args>
struct callable_traits<R (C::*)(Args...) const> {
    using return_type = R;
    using arg_types = std::tuple<Args...>;
    static constexpr std::size_t arity = sizeof...(Args);
};

template <typename C, typename R, typename... Args>
struct callable_traits<R (C::*)(Args...)> {
    using return_type = R;
    using arg_types = std::tuple<Args...>;
    static constexpr std::size_t arity = sizeof...(Args);
};

template <typename F>
struct callable_traits<F, std::void_t<decltype(&F::operator())>>
    : callable_traits<decltype(&F::operator())> {};

template <typename F>
using return_t = typename callable_traits<std::decay_t<F>>::return_type;

template <typename F>
constexpr std::size_t arity_v = callable_traits<std::decay_t<F>>::arity;

template <typename F, std::size_t I>
using arg_t =
    std::tuple_element_t<I,
                         typename callable_traits<std::decay_t<F>>::arg_types>;

} // namespace detail

class Defer;

namespace detail {

template <typename F, bool HasArgs>
struct first_arg_is_defer_impl : std::false_type {};

template <typename F>
struct first_arg_is_defer_impl<F, true>
    : std::bool_constant<
          std::is_same_v<std::decay_t<arg_t<F, 0>>, ::nv::Defer>> {};

template <typename F>
constexpr bool first_arg_is_defer_v =
    first_arg_is_defer_impl<F, (arity_v<F> > 0)>::value;

template <typename F>
constexpr std::size_t value_arity_v =
    arity_v<F> - (first_arg_is_defer_v<F> ? 1 : 0);

template <typename F, bool HasDefer = first_arg_is_defer_v<F>,
          std::size_t VA = value_arity_v<F>>
struct value_arg_type {
    using type = void;
};

template <typename F> struct value_arg_type<F, false, 1> {
    using type = arg_t<F, 0>;
};

template <typename F> struct value_arg_type<F, true, 1> {
    using type = arg_t<F, 1>;
};

template <typename F> using value_arg_t = typename value_arg_type<F>::type;

template <typename U> struct is_promise : std::false_type {};
template <typename U> struct is_promise<Promise<U>> : std::true_type {};
template <typename U> constexpr bool is_promise_v = is_promise<U>::value;

template <typename U> struct promise_value_type {
    using type = U;
};
template <typename U> struct promise_value_type<Promise<U>> {
    using type = U;
};
template <typename U>
using promise_value_t = typename promise_value_type<U>::type;

} // namespace detail

// ---------------------------------------------------------------------------
// Defer
// ---------------------------------------------------------------------------
class Defer {
  public:
    Defer() = default;
    explicit Defer(const RefPtr<PromiseBase>& promise) : _promise(promise) {}

    void resolve() const;
    void resolve(const Any& value) const;
    void reject() const;
    void reject(const Any& error) const;

  private:
    RefPtr<PromiseBase> _promise;
};

// ---------------------------------------------------------------------------
// PromiseState
// ---------------------------------------------------------------------------
enum class PromiseState { PENDING, RESOLVED, REJECTED };

// ---------------------------------------------------------------------------
// PromiseBase  –  type-erased core
// ---------------------------------------------------------------------------
class PromiseBase : public WeakRefObject {
    NV_DECLARE_CLASS(PromiseBase)

  public:
    using Job = JobDispatcher::Job;

    struct Continuation {
        Job job;
        bool onMain;
    };

    PromiseBase();
    ~PromiseBase() override = default;

    auto get_state() const -> PromiseState {
        return _state.load(std::memory_order_acquire);
    }
    auto is_pending() const -> bool {
        return get_state() == PromiseState::PENDING;
    }
    auto is_resolved() const -> bool {
        return get_state() == PromiseState::RESOLVED;
    }
    auto is_rejected() const -> bool {
        return get_state() == PromiseState::REJECTED;
    }

    auto get_value() const -> const Any& { return _value; }
    auto get_error() const -> const Any& { return _error; }

    void resolve_internal();
    void resolve_internal(const Any& value);
    void reject_internal();
    void reject_internal(const Any& error);

    // -----------------------------------------------------------------------
    // Cancellation support
    // -----------------------------------------------------------------------

    // Attach an external token.  If it is already cancelled when attached,
    // the promise is rejected immediately.
    void set_cancellation_token(const CancellationToken& token) {
        _cancellation_token = token;
        if (token.is_cancelled() && is_pending())
            reject_internal(Any(CancelledError{}));
    }

    const CancellationToken& get_cancellation_token() const {
        return _cancellation_token;
    }

    // Cancel this promise directly (creates an internal token state for it).
    // Rejects with CancelledError if still pending.
    void cancel() {
        if (is_pending())
            reject_internal(Any(CancelledError{}));
    }

    auto is_cancelled() const -> bool {
        return is_rejected() && _error.isA<CancelledError>();
    }

    // -----------------------------------------------------------------------

    void add_continuation(Continuation continuation);

  protected:
    void execute_continuations();

    std::atomic<PromiseState> _state{PromiseState::PENDING};
    Any _value;
    Any _error;

    // ← new member
    CancellationToken _cancellation_token;

    std::mutex _mutex;
    Vector<Continuation> _continuations;
};

// ---------------------------------------------------------------------------
// Invocation helpers (unchanged)
// ---------------------------------------------------------------------------
namespace detail {

template <typename ArgT> decltype(auto) cast_any(const Any& v) {
    if constexpr (std::is_same_v<std::decay_t<ArgT>, Any>)
        return v;
    else
        return v.get<std::decay_t<ArgT>>();
}

template <typename T> struct promise_value_access {
    using return_type =
        std::conditional_t<is_numeric_any_type_v<T>, T, const T&>;
};
template <> struct promise_value_access<void> {
    using return_type = const Any&;
};
template <> struct promise_value_access<Any> {
    using return_type = const Any&;
};

template <typename T>
using promise_return_t = typename promise_value_access<T>::return_type;

template <typename F>
auto invoke_callback(F&& func, Defer& defer, const Any& val) -> decltype(auto) {
    constexpr bool hasDefer = first_arg_is_defer_v<F>;
    constexpr bool hasValueArg = (value_arity_v<F> > 0);

    if constexpr (hasDefer) {
        if constexpr (hasValueArg) {
            using VA = value_arg_t<F>;
            return func(defer, cast_any<VA>(val));
        } else {
            return func(defer);
        }
    } else {
        if constexpr (hasValueArg) {
            using VA = value_arg_t<F>;
            return func(cast_any<VA>(val));
        } else {
            return func();
        }
    }
}

template <typename F, typename = void> struct is_callable : std::false_type {};

template <typename F>
struct is_callable<F, std::void_t<decltype(&std::decay_t<F>::operator())>>
    : std::true_type {};

// Also handle plain function pointers
template <typename R, typename... Args>
struct is_callable<R (*)(Args...)> : std::true_type {};

template <typename F>
constexpr bool is_callable_v = is_callable<std::decay_t<F>>::value;

template <typename F, typename = std::enable_if_t<is_callable_v<F>>>
using promise_continuation_t =
    Promise<std::conditional_t<first_arg_is_defer_v<F>, Any,
                               promise_value_t<return_t<F>>>>;

} // namespace detail

// ---------------------------------------------------------------------------
// SettledResult
// ---------------------------------------------------------------------------
struct SettledResult {
    PromiseState state;
    Any value;
    Any error;

    [[nodiscard]] auto is_resolved() const -> bool {
        return state == PromiseState::RESOLVED;
    }
    [[nodiscard]] auto is_rejected() const -> bool {
        return state == PromiseState::REJECTED;
    }
};

// ---------------------------------------------------------------------------
// Promise<T>
// ---------------------------------------------------------------------------
template <typename T = void> class Promise {
  public:
    using value_type = T;
    Promise() : _impl(create_ref_object<PromiseBase>()) {}
    explicit Promise(const RefPtr<PromiseBase>& impl) : _impl(impl) {}

    [[nodiscard]] auto is_pending() const -> bool {
        return _impl->is_pending();
    }
    [[nodiscard]] auto is_resolved() const -> bool {
        return _impl->is_resolved();
    }
    [[nodiscard]] auto is_rejected() const -> bool {
        return _impl->is_rejected();
    }

    // Returns true when rejected specifically due to cancellation.
    [[nodiscard]] auto is_cancelled() const -> bool {
        return _impl->is_cancelled();
    }

    // -----------------------------------------------------------------------
    // Cancellation
    // -----------------------------------------------------------------------

    // Cancel this promise.  Rejects with CancelledError if still pending.
    // Continuations chained with then() will also see a rejection and
    // propagate it without executing their callbacks (standard rejection
    // propagation already handles this).
    void cancel() { _impl->cancel(); }

    // Attach an external CancellationToken.  When the token is cancelled,
    // the promise will be rejected with CancelledError.
    // NOTE: this wires up cooperative cancellation — the promise is only
    // rejected when the token is checked (i.e. when a new continuation is
    // added, or when execute_continuations runs).  For immediate rejection
    // on cancel(), use a CancellationSource and call source.cancel() — then
    // call promise.cancel() yourself, or use make_promise(..., token).
    auto with_token(const CancellationToken& token) -> Promise<T>& {
        _impl->set_cancellation_token(token);
        return *this;
    }

    // -----------------------------------------------------------------------
    // get_value / get_error
    // -----------------------------------------------------------------------
    [[nodiscard]] auto get_value() const -> detail::promise_return_t<T> {
        NVCHK(is_resolved(), "Promise is not resolved");
        if constexpr (std::is_void_v<T> || std::is_same_v<std::decay_t<T>, Any>)
            return _impl->get_value();
        else
            return _impl->get_value().get<T>();
    }

    [[nodiscard]] auto get_error() const -> const Any& {
        NVCHK(is_rejected(), "Promise is not rejected");
        return _impl->get_error();
    }

    // Busy-wait until promise settles (for testing only).
    void await(F64 timeout_seconds = 5.0) const {
        auto start = std::chrono::high_resolution_clock::now();
        auto timeout = std::chrono::duration<F64>(timeout_seconds);
        while (is_pending()) {
            if (std::chrono::high_resolution_clock::now() - start > timeout)
                throw std::runtime_error("Promise await timeout");
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    // -----------------------------------------------------------------------
    // then()
    // -----------------------------------------------------------------------
    // Overload 1 – no token (existing behaviour, unchanged).
    template <typename F>
    auto then(F&& func) -> detail::promise_continuation_t<F> {
        using PType = detail::promise_continuation_t<F>::value_type;
        return then_impl<PType>(std::forward<F>(func),
                                CancellationToken::none());
    }

    // Overload 2 – with a CancellationToken.
    // The continuation is skipped (next promise rejected with CancelledError)
    // if the token is already cancelled when the continuation runs.
    template <typename F>
    auto then(F&& func, const CancellationToken& token)
        -> detail::promise_continuation_t<F> {
        using PType = detail::promise_continuation_t<F>::value_type;
        return then_impl<PType>(std::forward<F>(func), token);
    }

    template <typename U, typename F> auto then(F&& func) -> Promise<U> {
        return then_impl<U>(std::forward<F>(func), CancellationToken::none());
    }

    template <typename U, typename F>
    auto then(F&& func, const CancellationToken& token) -> Promise<U> {
        return then_impl<U>(std::forward<F>(func), token);
    }

    template <typename U, typename F>
    auto then_on_main(F&& func) -> Promise<U> {
        return then_impl<U>(std::forward<F>(func), CancellationToken::none(),
                            true);
    }

    template <typename F>
    auto then_on_main(F&& func) -> detail::promise_continuation_t<F> {
        using PType = detail::promise_continuation_t<F>::value_type;
        return then_impl<PType>(std::forward<F>(func),
                                CancellationToken::none(), true);
    }

    // Overload 2 – with a CancellationToken.
    // The continuation is skipped (next promise rejected with CancelledError)
    // if the token is already cancelled when the continuation runs.
    template <typename F>
    auto then_on_main(F&& func, const CancellationToken& token)
        -> detail::promise_continuation_t<F> {
        using PType = detail::promise_continuation_t<F>::value_type;
        return then_impl<PType>(std::forward<F>(func), token, true);
    }

    // -----------------------------------------------------------------------
    // catch_error()  –  unchanged, but CancelledError passes through cleanly
    // because it is just another Any value.
    // -----------------------------------------------------------------------
    template <typename F> auto catch_error(F&& func) -> Promise<T> {
        auto nextPromise = create_ref_object<PromiseBase>();

        _impl->add_continuation(
            {[func = std::forward<F>(func), impl = _impl,
              nextPromise]() mutable {
                 if (impl->is_resolved()) {
                     if constexpr (std::is_void_v<T>)
                         nextPromise->resolve_internal();
                     else
                         nextPromise->resolve_internal(impl->get_value());
                     return;
                 }

                 try {
                     if constexpr (std::is_void_v<
                                       std::invoke_result_t<F, const Any&>>) {
                         func(impl->get_error());
                         if constexpr (std::is_void_v<T>)
                             nextPromise->resolve_internal();
                         else
                             nextPromise->resolve_internal(impl->get_value());
                     } else {
                         auto result = func(impl->get_error());
                         nextPromise->resolve_internal(Any(std::move(result)));
                     }
                 } catch (...) {
                     nextPromise->reject_internal(
                         Any(std::current_exception()));
                 }
             },
             false});

        return Promise<T>(nextPromise);
    }

    // -----------------------------------------------------------------------
    // finally()
    // -----------------------------------------------------------------------
    template <typename F> auto finally(F&& func) -> Promise<T> {
        auto nextPromise = create_ref_object<PromiseBase>();

        _impl->add_continuation(
            {[func = std::forward<F>(func), impl = _impl,
              nextPromise]() mutable {
                 try {
                     func();
                     if (impl->is_resolved()) {
                         if constexpr (std::is_void_v<T>)
                             nextPromise->resolve_internal();
                         else
                             nextPromise->resolve_internal(impl->get_value());
                     } else {
                         nextPromise->reject_internal(impl->get_error());
                     }
                 } catch (...) {
                     nextPromise->reject_internal(
                         Any(std::current_exception()));
                 }
             },
             false});
        return Promise<T>(nextPromise);
    }

  private:
    RefPtr<PromiseBase> _impl;

    template <typename U> friend class Promise;
    friend class Defer;

    template <typename... Ps> friend auto promise_all(Ps&&...) -> Promise<void>;

    template <typename... Ps>
    friend auto promise_all_with_token(const CancellationToken&, Ps&&...)
        -> Promise<void>;

    template <typename U>
    friend auto promise_all(Vector<Promise<U>>) -> Promise<void>;

    template <typename U>
    friend auto promise_all_with_token(const CancellationToken&,
                                       Vector<Promise<U>>) -> Promise<void>;

    template <typename... Ps>
    friend auto promise_all_settled(Ps&&...) -> Promise<Vector<SettledResult>>;

    template <typename U>
    friend auto promise_all_settled(Vector<Promise<U>>)
        -> Promise<Vector<SettledResult>>;

    template <typename U2, typename... Ps>
    friend auto promise_race(Ps&&...) -> Promise<U2>;

    template <typename U2>
    friend auto promise_race(Vector<Promise<U2>>) -> Promise<U2>;

    // -----------------------------------------------------------------------
    // then_impl  –  shared implementation for both then() overloads.
    // -----------------------------------------------------------------------
    template <typename U, typename F>
    auto then_impl(F&& func, const CancellationToken& token,
                   bool onMain = false) -> Promise<U> {

        using R = detail::return_t<F>;
        constexpr bool hasDefer = detail::first_arg_is_defer_v<F>;
        using ResultValue =
            std::conditional_t<hasDefer, Any, detail::promise_value_t<R>>;

        auto nextPromise = create_ref_object<PromiseBase>();

        // Propagate the token to the next promise so the whole chain is
        // cancellable from a single source.
        if (token.is_cancelled()) {
            nextPromise->set_cancellation_token(token);
        }

        _impl->add_continuation(
            {[func = std::forward<F>(func), impl = _impl, nextPromise,
              token]() mutable {
                 // Check cancellation before doing any work.
                 if (token.is_cancelled()) {
                     nextPromise->reject_internal(Any(CancelledError{}));
                     return;
                 }

                 if (impl->is_rejected()) {
                     nextPromise->reject_internal(impl->get_error());
                     return;
                 }

                 try {
                     Defer defer(nextPromise);
                     const Any& val = impl->get_value();

                     if constexpr (hasDefer) {
                         detail::invoke_callback(func, defer, val);
                     } else {
                         if constexpr (std::is_void_v<R>) {
                             detail::invoke_callback(func, defer, val);
                             nextPromise->resolve_internal();
                         } else if constexpr (detail::is_promise_v<R>) {
                             R resultPromise =
                                 detail::invoke_callback(func, defer, val);

                             resultPromise._impl->add_continuation(
                                 {[nextPromise, rImpl = resultPromise._impl,
                                   token]() {
                                      // Also check token when the inner promise
                                      // settles.
                                      if (token.is_cancelled()) {
                                          nextPromise->reject_internal(
                                              Any(CancelledError{}));
                                          return;
                                      }
                                      if (rImpl->is_resolved())
                                          nextPromise->resolve_internal(
                                              rImpl->get_value());
                                      else
                                          nextPromise->reject_internal(
                                              rImpl->get_error());
                                  },
                                  false});
                         } else {
                             R result =
                                 detail::invoke_callback(func, defer, val);
                             nextPromise->resolve_internal(
                                 Any(std::move(result)));
                         }
                     }
                 } catch (const CancelledError&) {
                     // If the user's callback throws CancelledError directly,
                     // treat it the same as cancellation.
                     nextPromise->reject_internal(Any(CancelledError{}));
                 } catch (...) {
                     nextPromise->reject_internal(
                         Any(std::current_exception()));
                 }
             },
             onMain});

        return Promise<ResultValue>(nextPromise);
    }
};

// ---------------------------------------------------------------------------
// Factory functions
// ---------------------------------------------------------------------------

template <typename T>
inline auto make_resolved_promise(T&& value) -> Promise<std::decay_t<T>> {
    auto impl = create_ref_object<PromiseBase>();
    impl->resolve_internal(Any(std::forward<T>(value)));
    return Promise<std::decay_t<T>>(impl);
}

inline auto make_resolved_promise() -> Promise<void> {
    auto impl = create_ref_object<PromiseBase>();
    impl->resolve_internal();
    return Promise<void>(impl);
}

template <typename E, typename T = void>
inline auto make_rejected_promise(E&& error) -> Promise<T> {
    auto impl = create_ref_object<PromiseBase>();
    impl->reject_internal(Any(std::forward<E>(error)));
    return Promise<T>(impl);
}

template <typename T = void> inline auto make_rejected_promise() -> Promise<T> {
    auto impl = create_ref_object<PromiseBase>();
    impl->reject_internal();
    return Promise<T>(impl);
}

// ---------------------------------------------------------------------------
// make_promise  –  cooperative-cancellation overloads.
//
// Without token (original behaviour):
//   auto p = make_promise([](nv::Defer d) { ... });
//
// With token:
//   CancellationSource src;
//   auto p = make_promise([](nv::Defer d) {
//       // long async work…
//   }, src.token());
//   src.cancel();  // p (and its chain) reject with CancelledError
//
// The executor lambda can also call token.throw_if_cancelled() at any point
// to bail out early.
// ---------------------------------------------------------------------------
template <typename F> inline auto make_promise(F&& func) -> Promise<Any> {
    auto impl = create_ref_object<PromiseBase>();
    Defer defer(impl);
    try {
        func(defer);
    } catch (const CancelledError&) {
        impl->reject_internal(Any(CancelledError{}));
    } catch (...) {
        impl->reject_internal(Any(std::current_exception()));
    }
    return Promise<Any>(impl);
}

template <typename F>
inline auto make_promise(F&& func, const CancellationToken& token)
    -> Promise<Any> {
    auto impl = create_ref_object<PromiseBase>();
    impl->set_cancellation_token(token);

    // If already cancelled before we even start, reject immediately.
    if (token.is_cancelled())
        return Promise<Any>(impl); // already rejected by set_cancellation_token

    Defer defer(impl);
    try {
        func(defer);
    } catch (const CancelledError&) {
        impl->reject_internal(Any(CancelledError{}));
    } catch (...) {
        impl->reject_internal(Any(std::current_exception()));
    }
    return Promise<Any>(impl);
}

template <typename U, typename F>
inline auto make_promise(F&& func) -> Promise<U> {
    auto impl = create_ref_object<PromiseBase>();
    Defer defer(impl);
    try {
        func(defer);
    } catch (const CancelledError&) {
        impl->reject_internal(Any(CancelledError{}));
    } catch (...) {
        impl->reject_internal(Any(std::current_exception()));
    }
    return Promise<U>(impl);
}

template <typename U, typename F>
inline auto make_promise(F&& func, const CancellationToken& token)
    -> Promise<U> {
    auto impl = create_ref_object<PromiseBase>();
    impl->set_cancellation_token(token);

    if (token.is_cancelled())
        return Promise<U>(impl);

    Defer defer(impl);
    try {
        func(defer);
    } catch (const CancelledError&) {
        impl->reject_internal(Any(CancelledError{}));
    } catch (...) {
        impl->reject_internal(Any(std::current_exception()));
    }
    return Promise<U>(impl);
}

template <typename F>
inline auto make_promise_on_main(F&& func) -> Promise<Any> {
    auto impl = create_ref_object<PromiseBase>();
    run_main_task([func = std::forward<F>(func), impl]() {
        Defer defer(impl);
        try {
            func(defer);
        } catch (const CancelledError&) {
            impl->reject_internal(Any(CancelledError{}));
        } catch (...) {
            impl->reject_internal(Any(std::current_exception()));
        }
    });

    return Promise<Any>(impl);
}

// Typed return variant
template <typename U, typename F>
inline auto make_promise_on_main(F&& func) -> Promise<U> {
    auto impl = create_ref_object<PromiseBase>();
    run_main_task([func = std::forward<F>(func), impl]() {
        Defer defer(impl);
        try {
            func(defer);
        } catch (const CancelledError&) {
            impl->reject_internal(Any(CancelledError{}));
        } catch (...) {
            impl->reject_internal(Any(std::current_exception()));
        }
    });
    return Promise<U>(impl);
}

// With cancellation token
template <typename F>
inline auto make_promise_on_main(F&& func, const CancellationToken& token)
    -> Promise<Any> {
    auto impl = create_ref_object<PromiseBase>();
    impl->set_cancellation_token(token);

    if (token.is_cancelled())
        return Promise<Any>(impl); // already rejected

    run_main_task([func = std::forward<F>(func), impl, token]() {
        // Re-check on main thread — may have been cancelled while queued.
        if (token.is_cancelled()) {
            impl->reject_internal(Any(CancelledError{}));
            return;
        }
        Defer defer(impl);
        try {
            func(defer);
        } catch (const CancelledError&) {
            impl->reject_internal(Any(CancelledError{}));
        } catch (...) {
            impl->reject_internal(Any(std::current_exception()));
        }
    });
    return Promise<Any>(impl);
}

// Typed return + cancellation token
template <typename U, typename F>
inline auto make_promise_on_main(F&& func, const CancellationToken& token)
    -> Promise<U> {
    auto impl = create_ref_object<PromiseBase>();
    impl->set_cancellation_token(token);

    if (token.is_cancelled())
        return Promise<U>(impl);

    run_main_task([func = std::forward<F>(func), impl, token]() {
        if (token.is_cancelled()) {
            impl->reject_internal(Any(CancelledError{}));
            return;
        }
        Defer defer(impl);
        try {
            func(defer);
        } catch (const CancelledError&) {
            impl->reject_internal(Any(CancelledError{}));
        } catch (...) {
            impl->reject_internal(Any(std::current_exception()));
        }
    });
    return Promise<U>(impl);
}

inline void reject_promise() { throw std::runtime_error("Promise rejected."); }

// ---------------------------------------------------------------------------
// promise_all_impl  –  real implementation, operates on a span of promises.
// ---------------------------------------------------------------------------
namespace detail {

inline auto promise_all_impl(const CancellationToken& token,
                             Vector<RefPtr<PromiseBase>>&& impls)
    -> Promise<void> {
    auto impl = create_ref_object<PromiseBase>();
    impl->set_cancellation_token(token);

    if (impls.empty()) {
        impl->resolve_internal();
        return Promise<void>(impl);
    }

    if (token.is_cancelled())
        return Promise<void>(impl);

    struct SharedState {
        std::atomic<U32> count;
        std::atomic<bool> failed{false};
        WeakPtr<PromiseBase> impl;
        CancellationToken token;

        SharedState(U32 n, PromiseBase* p, CancellationToken t)
            : count(n), impl(p), token(std::move(t)) {}

        void on_one_settled(const Any* error) {
            if (error) {
                bool expected = false;
                if (failed.compare_exchange_strong(expected, true))
                    if (RefPtr<PromiseBase> p = impl.lock())
                        p->reject_internal(*error);
            }
            if (count.fetch_sub(1, std::memory_order_acq_rel) == 1)
                if (!failed.load(std::memory_order_acquire))
                    if (RefPtr<PromiseBase> p = impl.lock())
                        p->resolve_internal();
        }
    };

    auto state = std::make_shared<SharedState>(static_cast<U32>(impls.size()),
                                               impl.get(), token);

    for (auto& src : impls) {
        src->add_continuation({.job =
                                   [s = state, src]() mutable {
                                       if (src->is_resolved())
                                           s->on_one_settled(nullptr);
                                       else {
                                           const Any& err = src->get_error();
                                           s->on_one_settled(&err);
                                       }
                                   },
                               .onMain = false});
    }

    return Promise<void>(impl);
}

inline auto promise_all_settled_impl(Vector<RefPtr<PromiseBase>>&& impls)
    -> Promise<Vector<SettledResult>> {

    auto impl = create_ref_object<PromiseBase>();

    if (impls.empty()) {
        impl->resolve_internal(Any(Vector<SettledResult>{}));
        return Promise<Vector<SettledResult>>(impl);
    }

    struct SharedState {
        std::atomic<U32> count;
        std::mutex results_mutex;
        Vector<SettledResult> results;
        RefPtr<PromiseBase> impl;

        SharedState(U32 n, RefPtr<PromiseBase> p)
            : count(n), results(n), impl(std::move(p)) {}
    };

    const U32 n = static_cast<U32>(impls.size());
    auto state = std::make_shared<SharedState>(n, impl);

    for (U32 i = 0; i < n; ++i) {
        auto& src = impls[i];
        // reuse the Promise<Any> shell just to get then()/catch_error()
        Promise<Any> p(src);
        p.then([state, i](const Any& value) {
             {
                 std::lock_guard lock(state->results_mutex);
                 state->results[i] =
                     SettledResult{.state = PromiseState::RESOLVED,
                                   .value = value,
                                   .error = Any()};
             }
             if (state->count.fetch_sub(1, std::memory_order_acq_rel) == 1)
                 state->impl->resolve_internal(Any(state->results));
         }).catch_error([state, i](const Any& error) {
            {
                std::lock_guard lock(state->results_mutex);
                state->results[i] =
                    SettledResult{.state = PromiseState::REJECTED,
                                  .value = Any(),
                                  .error = error};
            }
            if (state->count.fetch_sub(1, std::memory_order_acq_rel) == 1)
                state->impl->resolve_internal(Any(state->results));
        });
    }

    return Promise<Vector<SettledResult>>(impl);
}

inline auto promise_race_impl(Vector<RefPtr<PromiseBase>>&& impls)
    -> Promise<Any> {
    auto impl = create_ref_object<PromiseBase>();
    auto settled = std::make_shared<std::atomic<bool>>(false);

    for (auto& src : impls) {
        Promise<Any> p(src);
        p.then([impl, settled](const Any& value) {
             bool expected = false;
             if (settled->compare_exchange_strong(expected, true))
                 impl->resolve_internal(value);
         }).catch_error([impl, settled](const Any& error) {
            bool expected = false;
            if (settled->compare_exchange_strong(expected, true))
                impl->reject_internal(error);
        });
    }

    return Promise<Any>(impl);
}

} // namespace detail

// ---------------------------------------------------------------------------
// promise_all()
// ---------------------------------------------------------------------------
template <typename... Promises>
inline auto promise_all(Promises&&... promises) -> Promise<void> {
    return detail::promise_all_impl(
        CancellationToken::none(), {std::forward<Promises>(promises)._impl...});
}

template <typename... Promises>
inline auto promise_all_with_token(const CancellationToken& token,
                                   Promises&&... promises) -> Promise<void> {
    return detail::promise_all_impl(
        token, {std::forward<Promises>(promises)._impl...});
}

template <typename T>
inline auto promise_all(Vector<Promise<T>> promises) -> Promise<void> {
    Vector<RefPtr<PromiseBase>> impls;
    impls.reserve(promises.size());
    for (auto& p : promises)
        impls.push_back(p._impl); // ← needs friend
    return detail::promise_all_impl(CancellationToken::none(),
                                    std::move(impls));
}

template <typename T>
inline auto promise_all_with_token(const CancellationToken& token,
                                   Vector<Promise<T>> promises)
    -> Promise<void> {
    Vector<RefPtr<PromiseBase>> impls;
    impls.reserve(promises.size());
    for (auto& p : promises)
        impls.push_back(p._impl); // ← needs friend
    return detail::promise_all_impl(token, std::move(impls));
}

// ---------------------------------------------------------------------------
// promise_all_settled()
// ---------------------------------------------------------------------------
template <typename... Promises>
inline auto promise_all_settled(Promises&&... promises)
    -> Promise<Vector<SettledResult>> {
    return detail::promise_all_settled_impl(
        {std::forward<Promises>(promises)._impl...});
}

template <typename T>
inline auto promise_all_settled(Vector<Promise<T>> promises)
    -> Promise<Vector<SettledResult>> {
    Vector<RefPtr<PromiseBase>> impls;
    impls.reserve(promises.size());
    for (auto& p : promises)
        impls.push_back(p._impl);
    return detail::promise_all_settled_impl(std::move(impls));
}

// ---------------------------------------------------------------------------
// promise_race()
// ---------------------------------------------------------------------------
template <typename T, typename... Promises>
inline auto promise_race(Promises&&... promises) -> Promise<T> {
    return Promise<T>(
        detail::promise_race_impl({std::forward<Promises>(promises)._impl...})
            ._impl);
}

template <typename T>
inline auto promise_race(Vector<Promise<T>> promises) -> Promise<T> {
    Vector<RefPtr<PromiseBase>> impls;
    impls.reserve(promises.size());
    for (auto& p : promises)
        impls.push_back(p._impl);
    return Promise<T>(detail::promise_race_impl(std::move(impls))._impl);
}

} // namespace nv

#endif // NV_PROMISE_