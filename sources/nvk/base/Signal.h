#ifndef _NV_SIGNAL_H_
#define _NV_SIGNAL_H_

#include <nvk/base/std_containers.h>

namespace nv {

// Function traits to extract argument types from callable objects
namespace detail {
template <typename T> struct function_traits;

// Function pointer
template <typename R, typename... Args> struct function_traits<R (*)(Args...)> {
    using args_tuple = std::tuple<Args...>;
};

// Member function pointer
template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...)> {
    using args_tuple = std::tuple<Args...>;
};

// const Member function pointer
template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...) const> {
    using args_tuple = std::tuple<Args...>;
};

// Callable objects (including lambdas)
template <typename T> struct function_traits {
  private:
    using callable_traits = function_traits<decltype(&T::operator())>;

  public:
    using args_tuple = typename callable_traits::args_tuple;
};

} // namespace detail

template <typename... Args> class Signal {
  private:
    // Base interface for slots
    struct SlotBase {
        virtual ~SlotBase() = default;
        virtual void call(Args... args) = 0;
        [[nodiscard]] virtual auto is_one_shot() const -> bool = 0;
    };

    template <typename T> struct MemberSlot : SlotBase {
        using MemberFn = void (T::*)(Args...);
        T* instance;
        MemberFn fn;
        bool one_shot;

        MemberSlot(T* inst, MemberFn f, bool one_shot = false)
            : instance(inst), fn(f), one_shot(one_shot) {}

        void call(Args... args) override { (instance->*fn)(args...); }

        [[nodiscard]] auto is_one_shot() const -> bool override {
            return one_shot;
        }
    };

    template <typename F> struct CallableSlot : SlotBase {
        F callable;
        bool one_shot;

        explicit CallableSlot(F&& f, bool one_shot = false)
            : callable(std::forward<F>(f)), one_shot(one_shot) {}

        void call(Args... args) override { callable(args...); }

        [[nodiscard]] auto is_one_shot() const -> bool override {
            return one_shot;
        }
    };

    Map<I32, std::unique_ptr<SlotBase>> slots;
    I32 nextId = 0;

  public:
    // Returns connection ID that can be used to disconnect later
    template <typename T>
    auto connect(T* instance, void (T::*fn)(Args...)) -> I32 {
        I32 id = nextId++;
        slots[id] = std::make_unique<MemberSlot<T>>(instance, fn);
        return id;
    }

    template <typename F> auto connect(F&& f) -> I32 {
        I32 id = nextId++;
        slots[id] =
            std::make_unique<CallableSlot<std::decay_t<F>>>(std::forward<F>(f));
        return id;
    }

    // One-shot connection for member functions
    template <typename T>
    auto connect_once(T* instance, void (T::*fn)(Args...)) -> I32 {
        I32 id = nextId++;
        slots[id] = std::make_unique<MemberSlot<T>>(instance, fn, true);
        return id;
    }

    // One-shot connection for callable objects
    template <typename F> auto connect_once(F&& f) -> I32 {
        I32 id = nextId++;
        slots[id] = std::make_unique<CallableSlot<std::decay_t<F>>>(
            std::forward<F>(f), true);
        return id;
    }

    void disconnect(I32 id) { slots.erase(id); }

    void emit(Args... args) {
        // Collect IDs of one-shot slots that need to be removed after calling
        Vector<I32> to_remove;

        for (const auto& pair : slots) {
            pair.second->call(args...);

            // If this is a one-shot connection, mark it for removal
            if (pair.second->is_one_shot()) {
                to_remove.push_back(pair.first);
            }
        }

        // Remove all one-shot connections that were triggered
        for (I32 id : to_remove) {
            disconnect(id);
        }
    }

    void operator()(Args... args) { emit(args...); }

    // Optional: Clear all connections
    void clear() { slots.clear(); }

    // Optional: Get number of connected slots
    [[nodiscard]] auto size() const -> size_t { return slots.size(); }
};

// Base class for type erasure
class SignalHolderBase {
  public:
    virtual ~SignalHolderBase() = default;
};

// Derived class that holds the actual Signal
template <typename... Args> class SignalHolder : public SignalHolderBase {
  public:
    Signal<Args...> signal;
};

class SignalMap {
  private:
    Map<StringID, std::unique_ptr<SignalHolderBase>> _signals;

  protected:
    // Helper struct to expand tuple to parameter pack
    template <typename Tuple> struct expand_tuple;

    template <typename... Args> struct expand_tuple<std::tuple<Args...>> {
        static auto connect(SignalMap& signals, StringID id, auto&& f) -> I32 {
            return signals.get_signal<Args...>(id).connect(
                std::forward<decltype(f)>(f));
        }

        static auto connect_once(SignalMap& signals, StringID id, auto&& f)
            -> I32 {
            return signals.get_signal<Args...>(id).connect_once(
                std::forward<decltype(f)>(f));
        }
    };

  public:
    // Get or create a signal of specific type
    template <typename... Args>
    auto get_signal(StringID id) -> Signal<Args...>& {
        auto it = _signals.find(id);
        if (it == _signals.end()) {
            auto holder = std::make_unique<SignalHolder<Args...>>();
            Signal<Args...>& signal = holder->signal;
            _signals[id] = std::move(holder);
            return signal;
        }

        // Downcast to the correct type
        auto* holder = dynamic_cast<SignalHolder<Args...>*>(it->second.get());
        if (!holder) {
            throw std::runtime_error("Signal type mismatch");
        }
        return holder->signal;
    }

    // Helper method to check if a signal exists
    [[nodiscard]] auto has_signal(StringID id) const -> bool {
        return _signals.find(id) != _signals.end();
    }

    // Remove a signal
    void remove_signal(StringID id) { _signals.erase(id); }

    // Clear all signals
    void clear() { _signals.clear(); }

    // Get number of signals
    [[nodiscard]] auto size() const -> size_t { return _signals.size(); }

    template <typename F> auto connect(StringID eventId, F&& func) -> I32 {
        using args_tuple =
            typename detail::function_traits<std::decay_t<F>>::args_tuple;

        return expand_tuple<args_tuple>::connect(*this, eventId,
                                                 std::forward<F>(func));
    }

    template <typename F> auto connect_once(StringID eventId, F&& func) -> I32 {
        using args_tuple =
            typename detail::function_traits<std::decay_t<F>>::args_tuple;

        return expand_tuple<args_tuple>::connect_once(*this, eventId,
                                                      std::forward<F>(func));
    }
};

}; // namespace nv
#endif