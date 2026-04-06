#ifndef _NV_SIGNAL_H_
#define _NV_SIGNAL_H_

#ifdef NV_SIGNAL_NO_STD_CONTAINERS
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace nv {
template <typename K, typename V> using Map = std::map<K, V>;
template <typename K, typename V> using UnorderedMap = std::unordered_map<K, V>;
template <typename V> using Vector = std::vector<V>;
using String = std::string;
} // namespace nv
#else
#include <nvk/base/std_containers.h>
#endif

#include <atomic>
#include <nvk/base/string_id.h>

namespace nv {

// ---------------------------------------------------------------------------
// Function traits — extract argument types from any callable
// ---------------------------------------------------------------------------
namespace detail {

template <typename T> struct function_traits;

template <typename R, typename... Args> struct function_traits<R (*)(Args...)> {
    using args_tuple = std::tuple<Args...>;
};

template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...)> {
    using args_tuple = std::tuple<Args...>;
};

template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...) const> {
    using args_tuple = std::tuple<Args...>;
};

template <typename T> struct function_traits {
  private:
    using callable_traits = function_traits<decltype(&T::operator())>;

  public:
    using args_tuple = typename callable_traits::args_tuple;
};

// Compile-time type IDs without RTTI
template <typename... Args> struct type_id_generator {
    static auto get_id() -> void* {
        static char id;
        return &id;
    }
};

} // namespace detail

// ---------------------------------------------------------------------------
// Signal<Args...>
// ---------------------------------------------------------------------------

template <typename... Args> class Signal {
  public:
    using SlotId = I32;

  private:
    // -- Slot storage --------------------------------------------------------
    // Flat vector of {id, callable} pairs — cache-friendly for emit().
    // Disconnect is O(n) but connections are rare vs. emits.

    struct SlotBase {
        virtual ~SlotBase() = default;
        virtual void call(Args&&... args) = 0;
    };

    template <typename T> struct MemberSlot : SlotBase {
        using MemberFn = void (T::*)(Args...);
        T* instance;
        MemberFn fn;
        MemberSlot(T* inst, MemberFn f) : instance(inst), fn(f) {}
        void call(Args&&... args) override {
            (instance->*fn)(std::forward<Args>(args)...);
        }
    };

    template <typename F> struct CallableSlot : SlotBase {
        F callable;
        explicit CallableSlot(F&& f) : callable(std::forward<F>(f)) {}
        void call(Args&&... args) override {
            callable(std::forward<Args>(args)...);
        }
    };

    struct Entry {
        SlotId id{0};
        std::unique_ptr<SlotBase> slot;
        bool one_shot{false};
    };

    Vector<Entry> _slots;
    SlotId _nextId{1};

    // Re-entrancy guard: when _emitDepth > 0 we defer structural changes.
    mutable I32 _emitDepth{0};
    Vector<SlotId> _pendingRemove;
    Vector<Entry> _pendingAdd;

    void flush_pending() {
        for (SlotId id : _pendingRemove)
            do_remove(id);
        _pendingRemove.clear();

        for (auto& e : _pendingAdd)
            _slots.push_back(std::move(e));
        _pendingAdd.clear();
    }

    void do_remove(SlotId id) {
        for (auto it = _slots.begin(); it != _slots.end(); ++it) {
            if (it->id == id) {
                _slots.erase(it);
                return;
            }
        }
    }

    auto add_entry(Entry e) -> SlotId {
        SlotId id = e.id;
        if (_emitDepth > 0)
            _pendingAdd.push_back(std::move(e));
        else
            _slots.push_back(std::move(e));
        return id;
    }

  public:
    // -- Connect (permanent) -------------------------------------------------

    template <typename T>
    auto connect(T* instance, void (T::*fn)(Args...)) -> SlotId {
        Entry e;
        e.id = _nextId++;
        e.slot = std::make_unique<MemberSlot<T>>(instance, fn);
        return add_entry(std::move(e));
    }

    template <typename F> auto connect(F&& f) -> SlotId {
        Entry e;
        e.id = _nextId++;
        e.slot =
            std::make_unique<CallableSlot<std::decay_t<F>>>(std::forward<F>(f));
        return add_entry(std::move(e));
    }

    // -- Connect (one-shot) --------------------------------------------------

    template <typename T>
    auto connect_once(T* instance, void (T::*fn)(Args...)) -> SlotId {
        Entry e;
        e.id = _nextId++;
        e.slot = std::make_unique<MemberSlot<T>>(instance, fn);
        e.one_shot = true;
        return add_entry(std::move(e));
    }

    template <typename F> auto connect_once(F&& f) -> SlotId {
        Entry e;
        e.id = _nextId++;
        e.slot =
            std::make_unique<CallableSlot<std::decay_t<F>>>(std::forward<F>(f));
        e.one_shot = true;
        return add_entry(std::move(e));
    }

    // -- Disconnect ----------------------------------------------------------

    void disconnect(SlotId id) {
        if (_emitDepth > 0)
            _pendingRemove.push_back(id);
        else
            do_remove(id);
    }

    // -- Emit ----------------------------------------------------------------

    void emit(Args... args) {
        ++_emitDepth;

        // Collect one-shot IDs without invalidating the loop.
        Vector<SlotId> oneShots;

        for (auto& e : _slots) {
            e.slot->call(std::forward<Args>(args)...);
            if (e.one_shot)
                oneShots.push_back(e.id);
        }

        --_emitDepth;

        for (SlotId id : oneShots)
            disconnect(id);

        if (_emitDepth == 0)
            flush_pending();
    }

    void operator()(Args... args) { emit(std::forward<Args>(args)...); }

    // -- Utilities -----------------------------------------------------------

    void clear() {
        if (_emitDepth > 0) {
            // Defer: mark every live slot for removal.
            for (auto& e : _slots)
                _pendingRemove.push_back(e.id);
        } else {
            _slots.clear();
        }
    }

    [[nodiscard]] auto size() const -> size_t { return _slots.size(); }
};

// ---------------------------------------------------------------------------
// SignalHolderBase / SignalHolder — type-erased wrappers for SignalMap
// ---------------------------------------------------------------------------

class SignalHolderBase {
  protected:
    using TypeIDPtr = void*;
    TypeIDPtr type_id;
    explicit SignalHolderBase(TypeIDPtr id) : type_id(id) {}

  public:
    virtual ~SignalHolderBase() = default;

    [[nodiscard]] auto get_type_id() const -> TypeIDPtr { return type_id; }

    template <typename... Args> [[nodiscard]] auto is_type() const -> bool {
        return type_id == detail::type_id_generator<Args...>::get_id();
    }
};

template <typename... Args> class SignalHolder : public SignalHolderBase {
  public:
    Signal<Args...> signal;
    SignalHolder()
        : SignalHolderBase(detail::type_id_generator<Args...>::get_id()) {}
};

// ---------------------------------------------------------------------------
// SignalMap — named collection of heterogeneous signals
// ---------------------------------------------------------------------------

class SignalMap {
  private:
    // UnorderedMap gives O(1) lookup vs O(log n) for Map — better for
    // per-frame signal access patterns.
    UnorderedMap<StringID, std::unique_ptr<SignalHolderBase>> _signals;

  protected:
    template <typename Tuple> struct expand_tuple;

    template <typename... Args> struct expand_tuple<std::tuple<Args...>> {
        template <typename F>
        static auto connect(SignalMap& signals, StringID id, F&& f) -> I32 {
            return signals.get_signal<Args...>(id).connect(
                std::forward<decltype(f)>(f));
        }

        template <typename F>
        static auto connect_once(SignalMap& signals, StringID id, F&& f)
            -> I32 {
            return signals.get_signal<Args...>(id).connect_once(
                std::forward<decltype(f)>(f));
        }
    };

  public:
    template <typename... Args>
    auto get_signal(StringID id) -> Signal<Args...>& {
        auto it = _signals.find(id);
        if (it == _signals.end()) {
            auto holder = std::make_unique<SignalHolder<Args...>>();
            Signal<Args...>& sig = holder->signal;
            _signals[id] = std::move(holder);
            return sig;
        }

        auto* base = it->second.get();
        if (!base->template is_type<Args...>())
            throw std::runtime_error("Signal type mismatch");

        return static_cast<SignalHolder<Args...>*>(base)->signal;
    }

    [[nodiscard]] auto has_signal(StringID id) const -> bool {
        return _signals.find(id) != _signals.end();
    }

    void remove_signal(StringID id) { _signals.erase(id); }

    void clear() { _signals.clear(); }

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

} // namespace nv
#endif