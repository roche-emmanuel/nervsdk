# Promise — NervSDK Async API Reference

> **Namespace:** `nv`  
> **Header:** `<nvk/task/Promise.h>`

---

## Table of Contents

1. [Overview](#overview)
2. [Core Concepts](#core-concepts)
   - [Promise States](#promise-states)
   - [The `Defer` Handle](#the-defer-handle)
   - [Type Erasure and `Any`](#type-erasure-and-any)
3. [Creating Promises](#creating-promises)
   - [`make_resolved_promise()`](#make_resolved_promise)
   - [`make_rejected_promise()`](#make_rejected_promise)
   - [`make_promise()`](#make_promise)
   - [`make_promise_on_main()`](#make_promise_on_main)
4. [Chaining — `then()`](#chaining--then)
   - [Return Value Propagation](#return-value-propagation)
   - [Returning a Nested Promise](#returning-a-nested-promise)
   - [Using `Defer` in Continuations](#using-defer-in-continuations)
   - [`then_on_main()`](#then_on_main)
5. [Error Handling](#error-handling)
   - [`catch_error()`](#catch_error)
   - [`finally()`](#finally)
   - [Exceptions in Callbacks](#exceptions-in-callbacks)
6. [Cancellation](#cancellation)
   - [`CancellationSource` and `CancellationToken`](#cancellationsource-and-cancellationtoken)
   - [Direct Promise Cancellation](#direct-promise-cancellation)
   - [Token-Based Cancellation](#token-based-cancellation)
   - [Propagation Through Chains](#propagation-through-chains)
   - [Detecting Cancellation in Error Handlers](#detecting-cancellation-in-error-handlers)
7. [Promise Combinators](#promise-combinators)
   - [`promise_all()`](#promise_all)
   - [`promise_all_with_token()`](#promise_all_with_token)
   - [`promise_all_settled()`](#promise_all_settled)
   - [`promise_race()`](#promise_race)
8. [Inspecting a Promise](#inspecting-a-promise)
9. [Awaiting (Testing Only)](#awaiting-testing-only)
10. [Thread Model](#thread-model)
11. [Reference Summary](#reference-summary)
12. [Complete Examples](#complete-examples)

---

## Overview

`Promise<T>` is a lightweight, header-only async abstraction for NervSDK. It models a value of type `T` that may not be available yet, along with the ability to chain transformations and error handlers on it — without blocking threads.

Key design goals:

- **Type-safe chaining** — `then()` infers return types at compile time.
- **Cooperative cancellation** — cancel a whole chain from a single `CancellationSource`.
- **Thread-flexible execution** — continuations run on a worker thread by default; `then_on_main()` schedules them on the main thread.
- **Zero heap allocation for synchronous chains** — resolved/rejected factory helpers settle the promise immediately.

---

## Core Concepts

### Promise States

Every promise is in exactly one of three states:

| State      | Description                                                 |
| ---------- | ----------------------------------------------------------- |
| `PENDING`  | The async work has not yet completed.                       |
| `RESOLVED` | The work succeeded; a value (possibly `void`) is available. |
| `REJECTED` | The work failed; an error `Any` is available.               |

State transitions are one-way and thread-safe (`std::atomic`).

```cpp
nv::Promise<int> p;
p.is_pending();   // true
p.is_resolved();  // false
p.is_rejected();  // false
```

### The `Defer` Handle

`Defer` is a write-once handle passed into the executor lambda of `make_promise()`. Call `resolve()` or `reject()` on it — exactly once — to settle the underlying promise.

```cpp
auto p = make_promise([](nv::Defer d) {
    // Some async operation…
    if (success)
        d.resolve(Any(result));
    else
        d.reject(Any(String("failure reason")));
});
```

`Defer` can be captured and called from any thread, including a background task or timer callback.

### Type Erasure and `Any`

Values flow through `PromiseBase` as `nv::Any` (a type-erased container). The typed `Promise<T>` wrapper extracts the stored value via `Any::get<T>()` on demand. When writing `then()` callbacks you generally work with the real C++ type; the `Any` layer is transparent.

---

## Creating Promises

### `make_resolved_promise()`

Creates a promise that is **already resolved** with the given value.

```cpp
// Typed
auto p = make_resolved_promise(42);         // Promise<int>
auto s = make_resolved_promise(String("ok")); // Promise<String>

// Void
auto v = make_resolved_promise();            // Promise<void>
```

### `make_rejected_promise()`

Creates a promise that is **already rejected** with the given error.

```cpp
auto p = make_rejected_promise(String("something went wrong")); // Promise<void>

// Supply an explicit value type if the chain expects one:
auto p2 = make_rejected_promise<int>(String("error"));          // Promise<int>
```

### `make_promise()`

Creates a promise whose settlement is deferred to the executor lambda. The lambda receives a `Defer` handle it must call (possibly asynchronously) to settle the promise.

**Untyped (returns `Promise<Any>`):**

```cpp
auto p = make_promise([](nv::Defer d) {
    auto task = make_task([d]() mutable {
        d.resolve(Any(123));
    });
    task->schedule();
});
```

**Typed (returns `Promise<U>`):**

```cpp
auto p = make_promise<int>([](nv::Defer d) {
    d.resolve(Any(42));
});
// p.get_value() returns int directly
```

**With cancellation token:**

```cpp
CancellationSource src;

auto p = make_promise([](nv::Defer d) {
    d.resolve(Any(99));
}, src.token());

// Cancelling before the executor runs rejects the promise immediately:
src.cancel();
```

> **Note:** If the token is already cancelled when `make_promise()` is called, the executor lambda is **never invoked** and the promise is rejected immediately with `CancelledError`.

### `make_promise_on_main()`

Like `make_promise()`, but the executor lambda is scheduled to run on the **main thread** via `JobDispatcher`.

```cpp
auto p = make_promise_on_main([](nv::Defer d) {
    // Guaranteed to run on main thread
    d.resolve(Any(42));
});

// Typed variant:
auto p2 = make_promise_on_main<int>([](nv::Defer d) {
    d.resolve(Any(99));
});

// With cancellation:
auto p3 = make_promise_on_main([](nv::Defer d) {
    d.resolve(Any(7));
}, src.token());
```

The token is checked **twice**: once before the task is queued, and once on the main thread before the executor runs. This closes the race window between queuing and execution.

---

## Chaining — `then()`

`then()` registers a callback to run when a promise resolves. It returns a **new promise** whose type is inferred from the callback's return type.

```cpp
// General form:
auto next = promise.then([](const ValueType& v) -> ReturnType {
    return transform(v);
});
```

### Return Value Propagation

The callback's return type determines the type of the next promise in the chain.

| Callback returns | Next promise type                      |
| ---------------- | -------------------------------------- |
| `void`           | `Promise<void>`                        |
| `T`              | `Promise<T>`                           |
| `Promise<T>`     | `Promise<T>` (flattened automatically) |
| _(uses `Defer`)_ | `Promise<Any>`                         |

```cpp
// void → int → string
auto p = make_resolved_promise()
    .then([]() -> int { return 42; })
    .then([](const int& v) -> String { return String::from(v); });
```

### Returning a Nested Promise

When a callback returns a `Promise<T>`, the chain automatically **flattens** it — the outer promise waits for the inner one before proceeding.

```cpp
auto p = make_resolved_promise(10)
    .then([](const int& v) -> Promise<int> {
        // Could be a genuinely async operation:
        return make_resolved_promise(v * 2);
    })
    .then([](const int& v) -> int {
        return v + 5; // v == 25
    });
```

### Using `Defer` in Continuations

A `then()` callback may take `nv::Defer` as its **first** argument to delay resolution of the next promise asynchronously.

```cpp
auto p = make_resolved_promise(100)
    .then([](nv::Defer d, const int& val) {
        auto task = make_task([d, val]() mutable {
            d.resolve(Any(val * 2));
        });
        task->schedule();
    });
// p is Promise<Any>; p.get_value().get<int>() == 200
```

### `then_on_main()`

Same as `then()` but the callback is dispatched to the **main thread**.

```cpp
auto p = make_resolved_promise(42)
    .then_on_main([](const int& val) -> int {
        // Runs on main thread
        return val * 2;
    });
```

Mix `then()` and `then_on_main()` freely in a chain:

```cpp
auto p = make_resolved_promise(10)
    .then([](const int& v) -> int { return v * 2; })          // worker thread
    .then_on_main([](const int& v) -> int { return v + 5; })  // main thread
    .then([](const int& v) -> int { return v * 3; });          // worker thread
```

`then_on_main()` also accepts a `CancellationToken` as a second argument (see [Cancellation](#cancellation)).

---

## Error Handling

### `catch_error()`

Registers a callback that runs when the promise is **rejected**. If the upstream promise resolves, the callback is skipped and the value passes through.

```cpp
auto p = make_rejected_promise(String("oops"))
    .catch_error([](const Any& error) {
        auto msg = error.get<String>();
        // Handle the error…
    });
// p is now resolved (catch_error converts rejection → resolution)
```

**Returning a recovery value:**

```cpp
auto p = make_rejected_promise(String("error"))
    .catch_error([](const Any&) -> int {
        return 100; // Recover with a default value
    })
    .then([](const int& v) -> int { return v + 23; }); // v == 123
```

### `finally()`

Registers a callback that runs **regardless** of whether the promise resolves or rejects. The callback takes no arguments and its return value is ignored. The promise's original state (resolved/rejected) is preserved.

```cpp
auto p = make_resolved_promise(42)
    .finally([]() {
        // Cleanup always runs
    })
    .then([](const int& v) { /* v == 42 */ });
```

```cpp
auto p = make_rejected_promise(String("fail"))
    .finally([]() { /* Cleanup */ });
// p is still rejected with "fail"
```

### Exceptions in Callbacks

Uncaught exceptions thrown inside a `then()`, `catch_error()`, or `finally()` callback automatically **reject** the next promise in the chain. They are stored as `std::exception_ptr` inside an `Any`.

```cpp
auto p = make_resolved_promise(10)
    .then([](const int&) -> int {
        throw std::runtime_error("something went wrong");
        return 99; // never reached
    })
    .catch_error([](const Any& err) {
        // err holds a std::exception_ptr
    });
```

Throwing `CancelledError` from a callback is treated as cancellation:

```cpp
.then([](const int&) -> int {
    throw CancelledError{};
    return 0;
})
// Next promise: is_cancelled() == true
```

---

## Cancellation

NervSDK promises support cooperative cancellation through `CancellationSource` / `CancellationToken` pairs.

### `CancellationSource` and `CancellationToken`

```cpp
CancellationSource src;
CancellationToken tok = src.token(); // cheap to copy, share freely

src.cancel();          // signals all tokens from this source
tok.is_cancelled();    // true
tok.throw_if_cancelled(); // throws CancelledError if cancelled
```

- `CancellationSource` is **move-only** — there is one clear owner.
- `CancellationToken` is **copyable** — pass freely into lambdas and callbacks.
- `CancellationToken::none()` returns a token that is never cancelled.

### Direct Promise Cancellation

Call `promise.cancel()` to immediately reject a **pending** promise with `CancelledError`. Calling it on an already-settled promise is a no-op.

```cpp
auto p = make_promise([](nv::Defer) { /* never resolves */ });
p.cancel();
p.is_cancelled(); // true
p.get_error().isA<CancelledError>(); // true
```

### Token-Based Cancellation

#### Attaching a token to an existing promise — `with_token()`

```cpp
CancellationSource src;
auto p = make_promise([](nv::Defer) { /* … */ });
p.with_token(src.token());

src.cancel(); // p will be rejected with CancelledError
```

If the token is already cancelled when `with_token()` is called, the promise is rejected immediately.

#### Passing a token to `make_promise()`

```cpp
CancellationSource src;

auto p = make_promise([](nv::Defer d) {
    d.resolve(Any(99));
}, src.token());
```

#### Passing a token to `then()`

```cpp
CancellationSource src;

auto p = make_resolved_promise(42)
    .then([](const int& v) -> int {
        return v * 2;
    }, src.token());
```

The token is checked **before** the callback runs. If it is already cancelled at that moment, the callback is skipped and the next promise is rejected with `CancelledError`.

### Propagation Through Chains

Cancellation propagates naturally: once a promise is rejected (for any reason, including cancellation), all downstream `then()` callbacks are skipped and the `CancelledError` propagates to the end of the chain.

```cpp
CancellationSource src;
std::atomic<int> steps{0};

auto p = make_resolved_promise(1)
    .then([&](const int& v) -> int {
        steps++; return v + 1; // runs (not yet cancelled)
    }, src.token())
    .then([&](const int& v) -> int {
        src.cancel();           // cancel mid-chain
        steps++; return v + 1; // this step still completes
    }, src.token())
    .then([&](const int& v) -> int {
        steps += 100;           // SKIPPED — token already cancelled
        return v + 1;
    }, src.token());

p.await();
// steps == 2, p.is_cancelled() == true
```

> **Key rule:** The token is evaluated at the **start** of each continuation, not when it is registered. A step that calls `src.cancel()` will complete; the _next_ step will be skipped.

### Detecting Cancellation in Error Handlers

`CancelledError` is a regular value stored in the rejection `Any`. Test for it in `catch_error()`:

```cpp
.catch_error([](const Any& err) {
    if (err.isA<CancelledError>())
        return; // silently swallow cancellation
    // handle real errors…
});
```

Use `promise.is_cancelled()` as a convenience check on a settled promise:

```cpp
if (p.is_cancelled()) { /* was cancelled */ }
if (p.is_rejected() && !p.is_cancelled()) { /* real error */ }
```

---

## Promise Combinators

### `promise_all()`

Resolves when **all** input promises resolve. Rejects immediately (with the first error) if **any** input rejects. The result promise is `Promise<void>`.

```cpp
auto p1 = make_resolved_promise();
auto p2 = make_resolved_promise();
auto all = promise_all(p1, p2);

all.await();
all.is_resolved(); // true
```

**Vector overload:**

```cpp
Vector<Promise<int>> promises = { … };
auto all = promise_all(promises);
```

### `promise_all_with_token()`

Like `promise_all()` but accepts a `CancellationToken` as the first argument. If the token is already cancelled, the combined promise is rejected immediately without examining the inputs.

```cpp
CancellationSource src;

auto all = promise_all_with_token(src.token(), p1, p2, p3);

src.cancel(); // rejects the combined promise
```

### `promise_all_settled()`

Waits for **all** input promises to settle regardless of outcome. Always resolves with a `Vector<SettledResult>` — one entry per input, in order.

```cpp
struct SettledResult {
    PromiseState state; // RESOLVED or REJECTED
    Any value;
    Any error;

    bool is_resolved() const;
    bool is_rejected() const;
};
```

```cpp
auto p1 = make_resolved_promise(42);
auto p2 = make_rejected_promise(String("error"));
auto p3 = make_resolved_promise(99);

auto p = promise_all_settled(p1, p2, p3);
p.await();

auto results = p.get_value(); // Vector<SettledResult>
results[0].is_resolved();             // true
results[0].value.get<int>();          // 42
results[1].is_rejected();             // true
results[1].error.get<String>();       // "error"
results[2].is_resolved();             // true
results[2].value.get<int>();          // 99
```

Combine with `then()` to act only on successful results:

```cpp
promise_all_settled(p1, p2, p3)
    .then([](const Vector<SettledResult>& results) {
        for (const auto& r : results) {
            if (r.is_resolved())
                process(r.value.get<int>());
        }
    });
```

**Contrast with `promise_all()`:**

|                        | `promise_all`    | `promise_all_settled`   |
| ---------------------- | ---------------- | ----------------------- |
| Rejects on first error | ✅               | ❌                      |
| Waits for all          | Only if no error | Always                  |
| Result type            | `void`           | `Vector<SettledResult>` |

### `promise_race()`

Resolves or rejects as soon as **any** input promise settles. Subsequent settlements are ignored.

```cpp
auto p1 = make_resolved_promise(1);          // wins immediately
auto p2 = make_promise([](nv::Defer d) {     // slower
    make_task([d]() mutable {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        d.resolve(Any(2));
    })->schedule();
});

auto winner = promise_race<int>(p1, p2);
winner.await();
winner.get_value(); // 1
```

Specify the result type explicitly as a template argument:

```cpp
auto p = promise_race<MyType>(p1, p2, p3);
```

---

## Inspecting a Promise

```cpp
Promise<int> p = /* … */;

p.is_pending();    // not yet settled
p.is_resolved();   // settled successfully
p.is_rejected();   // settled with an error
p.is_cancelled();  // rejected specifically with CancelledError

// Access value (only when resolved):
int v = p.get_value();

// Access error (only when rejected):
const Any& err = p.get_error();
bool isCancelled = err.isA<CancelledError>();
```

Calling `get_value()` on a non-resolved promise, or `get_error()` on a non-rejected promise, triggers an assertion (`NVCHK`).

---

## Awaiting (Testing Only)

`Promise<T>::await()` spins the calling thread until the promise settles. **Do not use in production code.** It is provided for unit tests where no event loop is running.

```cpp
p.await();                   // default 5 s timeout
p.await(10.0);               // custom timeout in seconds
// throws std::runtime_error on timeout
```

For promises that use `then_on_main()` or `make_promise_on_main()`, you must also pump the main thread while waiting:

```cpp
while (p.is_pending()) {
    NervApp::instance().process_step(); // pump main thread queue
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
}
```

---

## Thread Model

| API                      | Where callback runs                                                      |
| ------------------------ | ------------------------------------------------------------------------ |
| `then()`                 | Worker thread (via `JobDispatcher::post()`)                              |
| `then_on_main()`         | Main thread                                                              |
| `make_promise()`         | Executor runs on **calling thread**; `Defer` can be called from anywhere |
| `make_promise_on_main()` | Executor queued to main thread                                           |
| `catch_error()`          | Worker thread                                                            |
| `finally()`              | Worker thread                                                            |

Continuations registered on an already-settled promise are dispatched immediately via `JobDispatcher`. Continuations registered while the promise is still pending are stored and dispatched when it settles.

If `JobDispatcher::post()` is not overridden (e.g. in tests), it executes jobs synchronously on the calling thread — which is why synchronous chains work without any event loop.

---

## Reference Summary

### Factory Functions

| Function                        | Returns               | Description                          |
| ------------------------------- | --------------------- | ------------------------------------ |
| `make_resolved_promise(v)`      | `Promise<decay_t<T>>` | Pre-resolved with value              |
| `make_resolved_promise()`       | `Promise<void>`       | Pre-resolved (void)                  |
| `make_rejected_promise(e)`      | `Promise<T>`          | Pre-rejected with error              |
| `make_rejected_promise<T>()`    | `Promise<T>`          | Pre-rejected, no error value         |
| `make_promise(fn)`              | `Promise<Any>`        | Async executor, untyped              |
| `make_promise<U>(fn)`           | `Promise<U>`          | Async executor, typed                |
| `make_promise(fn, tok)`         | `Promise<Any>`        | Async executor with cancellation     |
| `make_promise<U>(fn, tok)`      | `Promise<U>`          | Async executor, typed + cancellation |
| `make_promise_on_main(fn)`      | `Promise<Any>`        | Executor on main thread              |
| `make_promise_on_main<U>(fn)`   | `Promise<U>`          | Executor on main thread, typed       |
| `make_promise_on_main(fn, tok)` | `Promise<Any>`        | Main thread + cancellation           |

### `Promise<T>` Methods

| Method                  | Description                                                     |
| ----------------------- | --------------------------------------------------------------- |
| `is_pending()`          | True if not yet settled                                         |
| `is_resolved()`         | True if settled successfully                                    |
| `is_rejected()`         | True if settled with error                                      |
| `is_cancelled()`        | True if rejected with `CancelledError`                          |
| `get_value()`           | Extract resolved value (asserts if not resolved)                |
| `get_error()`           | Extract rejection error (asserts if not rejected)               |
| `cancel()`              | Reject with `CancelledError` if still pending                   |
| `with_token(tok)`       | Attach external token; rejects immediately if already cancelled |
| `then(fn)`              | Chain a continuation on worker thread                           |
| `then(fn, tok)`         | Chain with cancellation check                                   |
| `then<U>(fn)`           | Chain with explicit result type                                 |
| `then_on_main(fn)`      | Chain a continuation on main thread                             |
| `then_on_main(fn, tok)` | Main thread + cancellation                                      |
| `catch_error(fn)`       | Handle rejection                                                |
| `finally(fn)`           | Always-run cleanup                                              |
| `await(timeout_s)`      | Spin-wait (tests only)                                          |

### Combinators

| Function                           | Returns                          | Description                   |
| ---------------------------------- | -------------------------------- | ----------------------------- |
| `promise_all(ps…)`                 | `Promise<void>`                  | All must resolve              |
| `promise_all(vec)`                 | `Promise<void>`                  | Vector overload               |
| `promise_all_with_token(tok, ps…)` | `Promise<void>`                  | All must resolve, cancellable |
| `promise_all_settled(ps…)`         | `Promise<Vector<SettledResult>>` | Wait for all, any outcome     |
| `promise_all_settled(vec)`         | `Promise<Vector<SettledResult>>` | Vector overload               |
| `promise_race<T>(ps…)`             | `Promise<T>`                     | First to settle wins          |
| `promise_race<T>(vec)`             | `Promise<T>`                     | Vector overload               |

---

## Complete Examples

### Example 1 — Basic async operation with error handling

```cpp
auto p = make_promise([](nv::Defer d) {
        auto task = make_task([d]() mutable {
            bool ok = do_work();
            if (ok)
                d.resolve(Any(String("done")));
            else
                d.reject(Any(String("work failed")));
        });
        task->schedule();
    })
    .then([](const String& result) -> int {
        log("Work result: {}", result);
        return 0;
    })
    .catch_error([](const Any& err) -> int {
        log("Error: {}", err.get<String>());
        return -1;
    })
    .finally([]() {
        log("Cleanup.");
    });
```

---

### Example 2 — Cancellable operation

```cpp
CancellationSource src;

auto p = make_promise([](nv::Defer d) {
        auto task = make_task([d]() mutable {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            d.resolve(Any(42));
        });
        task->schedule();
    }, src.token())
    .then([](const Any& v) -> int {
        return v.get<int>() * 2;
    }, src.token())
    .catch_error([](const Any& err) -> int {
        if (err.isA<CancelledError>()) {
            log("Operation was cancelled.");
            return -1;
        }
        log("Real error.");
        return -2;
    });

// Cancel after 100ms:
make_task([&src]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    src.cancel();
})->schedule();
```

---

### Example 3 — Parallel work with `promise_all_settled`

```cpp
auto load_config  = make_promise<Config>([](nv::Defer d) { /* … */ });
auto load_assets  = make_promise<Assets>([](nv::Defer d) { /* … */ });
auto load_network = make_promise<Network>([](nv::Defer d) { /* … */ });

promise_all_settled(load_config, load_assets, load_network)
    .then([](const Vector<SettledResult>& results) {
        if (results[0].is_resolved())
            apply_config(results[0].value.get<Config>());

        if (results[1].is_resolved())
            apply_assets(results[1].value.get<Assets>());
        else
            log("Assets failed: {}", results[1].error.get<String>());

        if (results[2].is_resolved())
            connect(results[2].value.get<Network>());
    });
```

---

### Example 4 — Main-thread UI update

```cpp
make_promise<Data>([](nv::Defer d) {
        // Heavy work on worker thread:
        auto task = make_task([d]() mutable {
            d.resolve(Any(fetch_data()));
        });
        task->schedule();
    })
    .then([](const Data& data) -> ProcessedData {
        return process(data); // worker thread
    })
    .then_on_main([](const ProcessedData& result) {
        ui_widget.set_data(result); // safely on main thread
    })
    .catch_error([](const Any& err) {
        log("Failed: {}", err.get<String>());
    });
```

---

### Example 5 — Custom struct as resolved value

```cpp
struct MyResult {
    I32  value1;
    bool value2;
};

auto p = make_promise<MyResult>([](nv::Defer d) {
    d.resolve(MyResult{.value1 = 42, .value2 = true});
});

p.get_value().value1; // 42
p.get_value().value2; // true
```

---
