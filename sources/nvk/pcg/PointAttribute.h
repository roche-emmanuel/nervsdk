#ifndef _NV_POINTATTRIBUTE_H_
#define _NV_POINTATTRIBUTE_H_

#include <nvk_common.h>

namespace nv {

class PointAttribute : public RefObject {
    NV_DECLARE_NO_COPY(PointAttribute)
    NV_DECLARE_NO_MOVE(PointAttribute)
  public:
    struct Traits {};

    template <typename T> class AttributeHolder;

    explicit PointAttribute(String name, Traits traits);
    ~PointAttribute() override;
    virtual void resize(U32 size) = 0;
    virtual auto size() const -> U64 = 0;
    virtual auto element_size() const -> U32 = 0;
    virtual void randomize() = 0;

    auto name() const -> const String& { return _name; }
    auto get_type_id() const -> StringID { return _typeId; }

    template <typename T> auto is_type() const -> bool {
        return _typeId == TypeId<T>::id;
    }

    // Set values (with type checking)
    template <typename T> void set_values(Vector<T>&& values) {
        NVCHK(_typeId == TypeId<T>::id,
              "PointAttribute::set_values: type mismatch.");
        static_cast<AttributeHolder<T>*>(this)->assign_values(
            std::move(values));
    }

    template <typename T> void set_values(const Vector<T>& values) {
        NVCHK(_typeId == TypeId<T>::id,
              "PointAttribute::set_values: type mismatch.");
        static_cast<AttributeHolder<T>*>(this)->assign_values(values);
    }

    // Get values (with type checking)
    template <typename T> auto get_values() -> Vector<T>& {
        NVCHK(_typeId == TypeId<T>::id,
              "PointAttribute::get_values: type mismatch.");
        return static_cast<AttributeHolder<T>*>(this)->retrieve_values();
    }

    template <typename T> auto get_values() const -> const Vector<T>& {
        NVCHK(_typeId == TypeId<T>::id,
              "PointAttribute::get_values: type mismatch.");
        return static_cast<const AttributeHolder<T>*>(this)->retrieve_values();
    }

    // Get single value at index
    template <typename T> auto get_value(U64 index) const -> const T& {
        NVCHK(_typeId == TypeId<T>::id,
              "PointAttribute::get_value: type mismatch.");
        return static_cast<const AttributeHolder<T>*>(this)->retrieve_value(
            index);
    }

    // Set single value at index
    template <typename T> void set_value(U64 index, T&& value) {
        NVCHK(_typeId == TypeId<T>::id,
              "PointAttribute::set_value: type mismatch.");
        static_cast<AttributeHolder<T>*>(this)->assign_value(
            index, std::forward<T>(value));
    }

    // Randomize with type checking and custom ranges
    template <typename T> void randomize_values(T min, T max) {
        NVCHK(_typeId == TypeId<T>::id,
              "PointAttribute::randomize_values: type mismatch.");
        static_cast<AttributeHolder<T>*>(this)->randomize_with_range(min, max);
    }

    void randomize_values(const Box4d& range);

    // Factory method
    template <typename T>
    static auto create(String name, U32 size, const T& value = T{},
                       Traits traits = {}) -> RefPtr<PointAttribute> {
        return nv::create<AttributeHolder<T>>(std::move(name), size, value,
                                              std::move(traits));
    }

    template <typename T>
    static auto create(String name, Vector<T> values, Traits traits = {})
        -> RefPtr<PointAttribute> {
        return nv::create<AttributeHolder<T>>(
            std::move(name), std::move(values), std::move(traits));
    }

  protected:
    Traits _traits;
    String _name;
    StringID _typeId{0};
};

// Default randomization traits - can be specialized for custom types
template <typename T> struct RandomizationTraits {
    static constexpr bool supported = false;
    static auto default_min() -> T { return T{}; }
    static auto default_max() -> T { return T{}; }
    static void fill(T* ptr, U32 count, const T& min, const T& max) {}
};

// Specialization for I32
template <> struct RandomizationTraits<I32> {
    static constexpr bool supported = true;
    static auto default_min() -> I32 { return 0; }
    static auto default_max() -> I32 { return 100; }
    static void fill(I32* ptr, U32 count, I32 min, I32 max) {
        RandGen::instance().uniform_int_array(ptr, count, min, max);
    }
};

// Specialization for I64
template <> struct RandomizationTraits<I64> {
    static constexpr bool supported = true;
    static auto default_min() -> I64 { return 0; }
    static auto default_max() -> I64 { return 100; }
    static void fill(I64* ptr, U32 count, I64 min, I64 max) {
        RandGen::instance().uniform_int_array(ptr, count, min, max);
    }
};

// Specialization for F32
template <> struct RandomizationTraits<F32> {
    static constexpr bool supported = true;
    static auto default_min() -> F32 { return 0.0f; }
    static auto default_max() -> F32 { return 1.0f; }
    static void fill(F32* ptr, U32 count, F32 min, F32 max) {
        RandGen::instance().uniform_real_array(ptr, count, min, max);
    }
};

// Specialization for F64
template <> struct RandomizationTraits<F64> {
    static constexpr bool supported = true;
    static auto default_min() -> F64 { return 0.0; }
    static auto default_max() -> F64 { return 1.0; }
    static void fill(F64* ptr, U32 count, F64 min, F64 max) {
        RandGen::instance().uniform_real_array(ptr, count, min, max);
    }
};

// Specialization for Vec2d
template <> struct RandomizationTraits<Vec2d> {
    static constexpr bool supported = true;
    static auto default_min() -> Vec2d { return Vec2d(0.0); }
    static auto default_max() -> Vec2d { return Vec2d(1.0); }
    static void fill(Vec2d* ptr, U32 count, const Vec2d& min,
                     const Vec2d& max) {
        RandGen::instance().uniform_real_array(ptr, count, min, max);
    }
};

// Specialization for Vec3d
template <> struct RandomizationTraits<Vec3d> {
    static constexpr bool supported = true;
    static auto default_min() -> Vec3d { return Vec3d(0.0); }
    static auto default_max() -> Vec3d { return Vec3d(1.0); }
    static void fill(Vec3d* ptr, U32 count, const Vec3d& min,
                     const Vec3d& max) {
        RandGen::instance().uniform_real_array(ptr, count, min, max);
    }
};

// Specialization for Vec4d
template <> struct RandomizationTraits<Vec4d> {
    static constexpr bool supported = true;
    static auto default_min() -> Vec4d { return Vec4d(0.0); }
    static auto default_max() -> Vec4d { return Vec4d(1.0); }
    static void fill(Vec4d* ptr, U32 count, const Vec4d& min,
                     const Vec4d& max) {
        RandGen::instance().uniform_real_array(ptr, count, min, max);
    }
};

template <typename T>
class PointAttribute::AttributeHolder : public PointAttribute {
  public:
    AttributeHolder(String name, U32 size, const T& value, Traits traits)
        : PointAttribute(std::move(name), std::move(traits)),
          _values(size, value) {
        _typeId = TypeId<T>::id;
    }

    AttributeHolder(String name, Vector<T> values, Traits traits)
        : PointAttribute(std::move(name), std::move(traits)),
          _values(std::move(values)) {
        _typeId = TypeId<T>::id;
    }

    void assign_values(Vector<T>&& values) { _values = std::move(values); }
    void assign_values(const Vector<T>& values) { _values = values; }

    auto retrieve_values() -> Vector<T>& { return _values; }
    auto retrieve_values() const -> const Vector<T>& { return _values; }

    auto retrieve_value(U64 index) const -> const T& {
        NVCHK(index < _values.size(),
              "PointAttribute::retrieve_value: index {} out of bounds (size: "
              "{})",
              index, _values.size());
        return _values[index];
    }

    void assign_value(U64 index, T&& value) {
        NVCHK(index < _values.size(),
              "PointAttribute::assign_value: index {} out of bounds (size: {})",
              index, _values.size());
        _values[index] = std::forward<T>(value);
    }

    void resize(U32 size) override { _values.resize(size); }
    auto size() const -> U64 override { return _values.size(); }
    auto element_size() const -> U32 override { return sizeof(T); }

    // Default randomization using traits defaults
    void randomize() override {
        if constexpr (RandomizationTraits<T>::supported) {
            randomize_with_range(RandomizationTraits<T>::default_min(),
                                 RandomizationTraits<T>::default_max());
        } else {
            NVCHK(false,
                  "PointAttribute::randomize: type '{}' does not support "
                  "randomization.",
                  TypeId<T>::id);
        }
    }

    // Randomization with custom range
    void randomize_with_range(const T& min, const T& max) {
        if constexpr (RandomizationTraits<T>::supported) {
            if (!_values.empty()) {
                RandomizationTraits<T>::fill(
                    _values.data(), static_cast<U32>(_values.size()), min, max);
            }
        } else {
            NVCHK(false,
                  "PointAttribute::randomize_with_range: type '{}' does not "
                  "support randomization.",
                  TypeId<T>::name);
        }
    }

  protected:
    Vector<T> _values;
};

using PointAttributeVector = Vector<RefPtr<PointAttribute>>;
using PointAttributeMap = UnorderedMap<String, RefPtr<PointAttribute>>;

} // namespace nv

#endif