#ifndef NV_ANY_H_
#define NV_ANY_H_

#include <nvk/base/RefObject.h>
#include <nvk/log/LogManager.h>

#include <nvk_type_ids.h>

namespace nv {
// First, define a type trait to identify numeric types that should return by
// value
template <typename T> struct is_numeric_any_type : std::false_type {};

// Specialize for all numeric types
template <> struct is_numeric_any_type<I8> : std::true_type {};
template <> struct is_numeric_any_type<U8> : std::true_type {};
template <> struct is_numeric_any_type<I16> : std::true_type {};
template <> struct is_numeric_any_type<U16> : std::true_type {};
template <> struct is_numeric_any_type<I32> : std::true_type {};
template <> struct is_numeric_any_type<U32> : std::true_type {};
template <> struct is_numeric_any_type<I64> : std::true_type {};
template <> struct is_numeric_any_type<U64> : std::true_type {};
template <> struct is_numeric_any_type<F32> : std::true_type {};
template <> struct is_numeric_any_type<F64> : std::true_type {};

template <typename T>
inline constexpr bool is_numeric_any_type_v = is_numeric_any_type<T>::value;

struct Any {
    StringID dtype{0};
    std::any data;

    Any() = default;
    template <typename T>
    Any(const T& value, StringID id = 0) : dtype(id), data(value) {
        if constexpr (is_numeric_any_type_v<T>) {
            dtype = NV_TYPE_ID(T);
        }
    };

    // Check if this Any has a value.
    [[nodiscard]] auto is_empty() const -> bool { return !data.has_value(); }

    template <typename T> [[nodiscard]] auto isA() const -> bool;

    template <typename T>
    auto get() const
        -> std::conditional_t<is_numeric_any_type_v<T>, T, const T&>;

    template <typename T>
    auto get() -> std::conditional_t<is_numeric_any_type_v<T>, T, T&>;
    template <typename T> auto get_value() const -> T;

    template <typename T> auto get(const T& defVal) const -> T;

    template <typename T> void set(T&& value, StringID dt = 0);
};

template <typename T> struct AnyGetter {
    static auto get(const Any& val) -> const T& {
        if (!val.data.has_value())
            THROW_MSG("Cannot get from empty any.");

        if constexpr (is_vector_v<T>) {
            // Handle Vector<U> types
            const T* ref = std::any_cast<T>(&val.data);
            if (!ref)
                THROW_MSG("Invalid any cast.");
            return *ref;
        } else if constexpr (std::is_base_of_v<RefObject, T>) {
            const RefPtr<T>* refPtr = std::any_cast<RefPtr<T>>(&val.data);
            if (!refPtr)
                THROW_MSG("Invalid any cast.");
            return *refPtr->get();
        } else {
            const T* ref = std::any_cast<T>(&val.data);
            if (!ref)
                THROW_MSG("Invalid any cast.");
            return *ref;
        }
    }

    static auto get(Any& val) -> T& {
        if (!val.data.has_value())
            THROW_MSG("Cannot get from empty any.");

        if constexpr (is_vector_v<T>) {
            // Handle Vector<U> types
            T* ref = std::any_cast<T>(&val.data);
            if (!ref)
                THROW_MSG("Invalid any cast.");
            return *ref;
        } else if constexpr (std::is_base_of_v<RefObject, T>) {
            RefPtr<T>* refPtr = std::any_cast<RefPtr<T>>(&val.data);
            if (!refPtr)
                THROW_MSG("Invalid any cast.");
            return *refPtr->get();
        } else {
            T* ref = std::any_cast<T>(&val.data);
            if (!ref)
                THROW_MSG("Invalid any cast.");
            return *ref;
        }
    }
};

// Type ID definition:
NV_DEFINE_TYPE_ID(nv::Any)

template <> struct TypeId<Vector<bool>> {
    constexpr static nv::StringID id = str_id_const("nv::Vector<bool>");
};

template <> struct TypeId<Vector<I64>> {
    constexpr static nv::StringID id = str_id_const("nv::Vector<I64>");
};

template <> struct TypeId<Vector<F64>> {
    constexpr static nv::StringID id = str_id_const("nv::Vector<F64>");
};

template <> struct TypeId<Vector<String>> {
    constexpr static nv::StringID id = str_id_const("nv::Vector<String>");
};

template <typename T> struct AnyChecker {
    // Check if the provided any is of the given type:
    static auto isA(const Any& val) -> bool {
        if (val.dtype == 0) {
            // Try casting directly:
            return std::any_cast<T>(&val.data) != nullptr;
        }
        return val.dtype == TypeId<T>::id;
    }
};

template <> struct AnyChecker<I8> {
    // Check if the provided any is of the given type:
    static auto isA(const Any& val) -> bool {
        return val.dtype == TypeId<I8>::id || val.dtype == TypeId<U8>::id;
    }
};

template <> struct AnyChecker<U8> {
    // Check if the provided any is of the given type:
    static auto isA(const Any& val) -> bool {
        return val.dtype == TypeId<I8>::id || val.dtype == TypeId<U8>::id;
    }
};

template <> struct AnyChecker<I16> {
    // Check if the provided any is of the given type:
    static auto isA(const Any& val) -> bool {
        if (val.dtype == TypeId<I16>::id || val.dtype == TypeId<U16>::id) {
            return true;
        }
        return val.isA<I8>();
    }
};

template <> struct AnyChecker<U16> {
    // Check if the provided any is of the given type:
    static auto isA(const Any& val) -> bool {
        if (val.dtype == TypeId<I16>::id || val.dtype == TypeId<U16>::id) {
            return true;
        }
        return val.isA<U8>();
    }
};

template <> struct AnyChecker<I32> {
    // Check if the provided any is of the given type:
    static auto isA(const Any& val) -> bool {
        if (val.dtype == TypeId<I32>::id || val.dtype == TypeId<U32>::id) {
            return true;
        }
        return val.isA<I16>();
    }
};

template <> struct AnyChecker<U32> {
    // Check if the provided any is of the given type:
    static auto isA(const Any& val) -> bool {
        if (val.dtype == TypeId<I32>::id || val.dtype == TypeId<U32>::id) {
            return true;
        }
        return val.isA<U16>();
    }
};

template <> struct AnyChecker<I64> {
    // Check if the provided any is of the given type:
    static auto isA(const Any& val) -> bool {
        if (val.dtype == TypeId<I64>::id || val.dtype == TypeId<U64>::id) {
            return true;
        }
        return val.isA<I32>();
    }
};

template <> struct AnyChecker<U64> {
    // Check if the provided any is of the given type:
    static auto isA(const Any& val) -> bool {
        if (val.dtype == TypeId<I64>::id || val.dtype == TypeId<U64>::id) {
            return true;
        }
        return val.isA<U32>();
    }
};

template <> struct AnyChecker<F32> {
    // Check if the provided any is of the given type:
    static auto isA(const Any& val) -> bool {
        return val.dtype == TypeId<F32>::id || val.isA<I32>();
    }
};

template <> struct AnyChecker<F64> {
    // Check if the provided any is of the given type:
    static auto isA(const Any& val) -> bool {
        return val.dtype == TypeId<F64>::id || val.isA<I64>() || val.isA<F32>();
    }
};

template <> struct AnyChecker<Vector<F64>> {
    static auto isA(const Any& val) -> bool {
        return val.dtype == TypeId<Vector<F64>>::id ||
               val.dtype == TypeId<Vector<I64>>::id;
    }
};

template <> struct AnyGetter<U8> {
    static auto get(const Any& val) -> U8 {

        switch (val.dtype) {
        case NV_TYPE_ID(U8): {
            const U8* ref = std::any_cast<U8>(&val.data);
            NVCHK(ref != nullptr, "Invalid any_cast to U8");
            return (U8)*ref;
        }
        case NV_TYPE_ID(I8): {
            const I8* ref = std::any_cast<I8>(&val.data);
            NVCHK(ref != nullptr, "Invalid any_cast to I8");
            return (U8)*ref;
        }
        }

        THROW_MSG("Invalid number cast from Any.");
        return 0;
    }
};

template <> struct AnyGetter<I8> {
    static auto get(const Any& val) -> I8 {

        switch (val.dtype) {
        case NV_TYPE_ID(U8): {
            const U8* ref = std::any_cast<U8>(&val.data);
            NVCHK(ref != nullptr, "Invalid any_cast to U8");
            return (I8)*ref;
        }
        case NV_TYPE_ID(I8): {
            const I8* ref = std::any_cast<I8>(&val.data);
            NVCHK(ref != nullptr, "Invalid any_cast to I8");
            return (I8)*ref;
        }
        }

        THROW_MSG("Invalid number cast from Any.");
        return 0;
    }
};

template <typename T1, typename T2, typename SubT> struct IntGetter {
    static auto get(const Any& val) -> T1 {
        switch (val.dtype) {
        case NV_TYPE_ID(T1): {
            const T1* ref = std::any_cast<T1>(&val.data);
            NVCHK(ref != nullptr, "Invalid any_cast to {}",
                  std::string(typeid(T1).name()));
            return static_cast<T1>(*ref);
        }
        case NV_TYPE_ID(T2): {
            const T2* ref = std::any_cast<T2>(&val.data);
            NVCHK(ref != nullptr, "Invalid any_cast to {}",
                  std::string(typeid(T2).name()));
            return static_cast<T1>(*ref);
        }
        default:
            return static_cast<T1>(AnyGetter<SubT>::get(val));
        }
    }
};

// Specialize AnyGetter for each type using the generic template
template <> struct AnyGetter<U16> : IntGetter<U16, I16, U8> {};
template <> struct AnyGetter<I16> : IntGetter<I16, U16, I8> {};
template <> struct AnyGetter<U32> : IntGetter<U32, I32, U16> {};
template <> struct AnyGetter<I32> : IntGetter<I32, U32, I16> {};
template <> struct AnyGetter<U64> : IntGetter<U64, I64, U32> {};
template <> struct AnyGetter<I64> : IntGetter<I64, U64, I32> {};

template <> struct AnyGetter<F32> {
    static auto get(const Any& val) -> F32 {
        if (val.dtype == NV_TYPE_ID(F32)) {
            const F32* ref = std::any_cast<F32>(&val.data);
            NVCHK(ref != nullptr, "Invalid any_cast result.");
            return *ref;
        }
        if (val.dtype == NV_TYPE_ID(F64)) {
            const F64* ref = std::any_cast<F64>(&val.data);
            NVCHK(ref != nullptr, "Invalid any_cast result.");
            return (F32)*ref;
        }

        // Support converting I64 values:
        return (F32)AnyGetter<I64>::get(val);
    }
};

template <> struct AnyGetter<F64> {
    static auto get(const Any& val) -> F64 {

        if (val.dtype == NV_TYPE_ID(F64)) {
            const F64* ref = std::any_cast<F64>(&val.data);
            NVCHK(ref != nullptr, "Invalid any_cast result.");
            return *ref;
        }
        if (val.dtype == NV_TYPE_ID(F32)) {
            const F32* ref = std::any_cast<F32>(&val.data);
            NVCHK(ref != nullptr, "Invalid any_cast result.");
            return (F64)*ref;
        }

        return (F64)AnyGetter<I64>::get(val);
    }
};

template <> struct AnyGetter<F64Vector> {
    static auto get(const Any& val) -> const F64Vector& {

        if (val.dtype == TypeId<F64Vector>::id) {
            const auto* ref = std::any_cast<F64Vector>(&val.data);
            NVCHK(ref != nullptr, "Invalid any_cast result.");
            return *ref;
        }

        if (val.dtype != TypeId<I64Vector>::id) {
            THROW_MSG("AnyGetter: invalid type to extract F64Vector.");
        }

        const auto* ref = std::any_cast<I64Vector>(&val.data);
        NVCHK(ref != nullptr, "Invalid any_cast result.");

        logWARN("AnyGetter: Converting I64 array to F64 array.");

        // Convert and store in the Any object
        F64Vector converted;
        converted.reserve(ref->size());
        converted.insert(converted.end(), ref->begin(), ref->end());

        Any& mval = const_cast<Any&>(val);

        mval.data = converted;
        mval.dtype = TypeId<F64Vector>::id;

        const auto* result = std::any_cast<F64Vector>(&val.data);
        NVCHK(result != nullptr, "Invalid any_cast result after conversion.");
        return *result;
    }
};

template <> struct AnyChecker<Vec3d> {
    // Check if the provided any is of the given type:
    static auto isA(const Any& val) -> bool {
        if (AnyChecker<F64Vector>::isA(val)) {
            return AnyGetter<F64Vector>::get(val).size() == 3;
        }
        return val.dtype == TypeId<Vec3d>::id;
    }
};

template <> struct AnyChecker<Vec3f> {
    // Check if the provided any is of the given type:
    static auto isA(const Any& val) -> bool {
        if (AnyChecker<F64Vector>::isA(val)) {
            return AnyGetter<F64Vector>::get(val).size() == 3;
        }
        return val.dtype == TypeId<Vec3d>::id || val.dtype == TypeId<Vec3f>::id;
    }
};

template <typename T> static auto make_any(T&& value) -> Any {
    return Any{std::forward<T>(value), TypeId<T>::id};
}

template <typename T> static auto make_any(const T& value) -> Any {
    return Any{value, TypeId<T>::id};
}

template <typename T> [[nodiscard]] auto Any::isA() const -> bool {
    return AnyChecker<T>::isA(*this);
}

template <typename T>
auto Any::get() const
    -> std::conditional_t<is_numeric_any_type_v<T>, T, const T&> {
    return AnyGetter<T>::get(*this);
}

template <typename T>
auto Any::get() -> std::conditional_t<is_numeric_any_type_v<T>, T, T&> {
    return AnyGetter<T>::get(*this);
}

template <typename T> auto Any::get_value() const -> T {
    return AnyGetter<T>::get(*this);
}

template <typename T> auto Any::get(const T& defVal) const -> T {
    if (isA<T>()) {
        return get<T>();
    }
    return defVal;
}

template <typename T> void Any::set(T&& value, StringID dt) {
    // Update the data type identifier
    dtype = dt;

    // Assign the value to the any container
    data = std::forward<T>(value);
#ifdef DEBUG
    NVCHK(data.has_value(), "No value was stored in Any container!");
#endif
}

} // namespace nv

#endif
