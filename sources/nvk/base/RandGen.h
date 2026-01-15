#ifndef RANDGEN_H_
#define RANDGEN_H_

namespace nv {
class RandGen {
  public:
    explicit RandGen() : _gen(std::random_device{}()) {}
    explicit RandGen(U32 seed) : _gen(seed) {}

    static auto instance() -> const RandGen& {
        static RandGen obj(1234);
        return obj;
    }

    // Generate a random uniform float in the range [min, max]
    template <typename T>
    [[nodiscard]] auto uniform_real(T min = 0.0F, T max = 1.0F) const -> T {
        return min + static_cast<T>(_udis(_gen)) * (max - min);
    }

    template <typename T>
    void uniform_real_array(T* ptr, U32 num, T min = 0.0F, T max = 1.0F) const {
        T range = max - min;
        for (U32 i = 0; i < num; ++i) {
            ptr[i] = min + static_cast<T>(_udis(_gen)) * range;
        }
    }

    // Vec2 array filler
    template <typename T>
    void uniform_real_array(Vec2<T>* ptr, U32 num, Vec2<T> min = Vec2<T>(0.0F),
                            Vec2<T> max = Vec2<T>(1.0F)) const {
        Vec2<T> range = max - min;
        for (U32 i = 0; i < num; ++i) {
            ptr[i].set(min.x() + static_cast<T>(_udis(_gen)) * range.x(),
                       min.y() + static_cast<T>(_udis(_gen)) * range.y());
        }
    }

    // Vec3 array filler
    template <typename T>
    void uniform_real_array(Vec3<T>* ptr, U32 num, Vec3<T> min = Vec3<T>(0.0F),
                            Vec3<T> max = Vec3<T>(1.0F)) const {
        Vec3<T> range = max - min;
        for (U32 i = 0; i < num; ++i) {
            ptr[i].set(min.x() + static_cast<T>(_udis(_gen)) * range.x(),
                       min.y() + static_cast<T>(_udis(_gen)) * range.y(),
                       min.z() + static_cast<T>(_udis(_gen)) * range.z());
        }
    }

    // Vec4 array filler
    template <typename T>
    void uniform_real_array(Vec4<T>* ptr, U32 num, Vec4<T> min = Vec4<T>(0.0F),
                            Vec4<T> max = Vec4<T>(1.0F)) const {
        Vec4<T> range = max - min;
        for (U32 i = 0; i < num; ++i) {
            ptr[i].set(min.x() + static_cast<T>(_udis(_gen)) * range.x(),
                       min.y() + static_cast<T>(_udis(_gen)) * range.y(),
                       min.z() + static_cast<T>(_udis(_gen)) * range.z(),
                       min.w() + static_cast<T>(_udis(_gen)) * range.w());
        }
    }

    template <typename T>
    [[nodiscard]] auto uniform_int(T min, T max) const -> T {
        std::uniform_int_distribution<T> dis(min, max);
        return dis(_gen);
    }

    template <typename T>
    [[nodiscard]] auto uniform_int_vector(U32 count, T min, T max) const
        -> Vector<T> {
        Vector<T> res(count);
        std::uniform_int_distribution<T> dis(min, max);
        std::generate(res.begin(), res.end(),
                      [&dis, this]() { return dis(_gen); });
        return res;
    }

    template <>
    [[nodiscard]] auto uniform_int_vector<U8>(U32 count, U8 min, U8 max) const
        -> Vector<U8> {
        Vector<U8> res(count);
        F64 range = static_cast<F64>(max) - static_cast<F64>(min);
        std::generate(res.begin(), res.end(), [this, min, range]() {
            return static_cast<U8>(static_cast<F64>(min) + _udis(_gen) * range);
        });
        return res;
    }

    template <typename T>
    [[nodiscard]] auto uniform_real_vector(U32 count, T min, T max) const
        -> Vector<T> {
        Vector<T> res(count);
        T range = max - min;
        std::generate(res.begin(), res.end(), [this, min, range]() {
            return min + static_cast<T>(_udis(_gen)) * range;
        });
        return res;
    }

  private:
    mutable std::mt19937 _gen;
    mutable std::uniform_real_distribution<F64> _udis{0.0, 1.0};
};

// Generates a 4x4 matrix of random floats
template <typename T> auto gen_mat4(T mini = -1.0, T maxi = 1.0) -> Mat4<T> {
    Mat4<T> mat;
    RandGen::instance().uniform_real_array(mat.ptr(), 16, mini, maxi);
    return mat;
}

inline auto gen_mat4f(F32 mini = -1.0, F32 maxi = 1.0) -> Mat4f {
    return gen_mat4<F32>(mini, maxi);
}

inline auto gen_mat4d(F64 mini = -1.0, F64 maxi = 1.0) -> Mat4d {
    return gen_mat4<F64>(mini, maxi);
}

inline auto gen_vec4d(F64 mini = -1.0, F64 maxi = 1.0) -> Vec4d {
    Vec4d res;
    RandGen::instance().uniform_real_array(res._v.data(), 4, mini, maxi);
    return res;
}

inline auto gen_vec4f(F32 mini = -1.0, F32 maxi = 1.0) -> Vec4f {
    Vec4f res;
    RandGen::instance().uniform_real_array(res._v.data(), 4, mini, maxi);
    return res;
}

inline auto gen_vec3d(F64 mini = -1.0, F64 maxi = 1.0) -> Vec3d {
    Vec3d res;
    RandGen::instance().uniform_real_array(res._v.data(), 3, mini, maxi);
    return res;
}

inline auto gen_vec3f(F32 mini = -1.0, F32 maxi = 1.0) -> Vec3f {
    Vec3f res;
    RandGen::instance().uniform_real_array(res._v.data(), 3, mini, maxi);
    return res;
}

inline auto gen_vec2d(F64 mini = -1.0, F64 maxi = 1.0) -> Vec2d {
    Vec2d res;
    RandGen::instance().uniform_real_array(res._v.data(), 2, mini, maxi);
    return res;
}

inline auto gen_vec2f(F32 mini = -1.0, F32 maxi = 1.0) -> Vec2f {
    Vec2f res;
    RandGen::instance().uniform_real_array(res._v.data(), 2, mini, maxi);
    return res;
}

inline auto gen_f32(F32 mini = -1.0, F32 maxi = 1.0) -> F32 {
    return RandGen::instance().uniform_real<F32>(mini, maxi);
}

inline auto gen_f32(const Vec2f& range) -> F32 {
    return RandGen::instance().uniform_real<F32>(range.x(), range.y());
}

inline auto gen_f64(F64 mini = -1.0, F64 maxi = 1.0) -> F64 {
    return RandGen::instance().uniform_real<F64>(mini, maxi);
}

inline auto gen_f64(const Vec2d& range) -> F64 {
    return RandGen::instance().uniform_real<F64>(range.x(), range.y());
}

inline auto gen_u32(U32 mini = 0, U32 maxi = 100) -> U32 {
    return RandGen::instance().uniform_int<U32>(mini, maxi);
}

inline auto gen_u32(const Vec2u& range) -> U32 {
    return RandGen::instance().uniform_int<U32>(range.x(), range.y());
}

inline auto gen_i32(I32 mini = 0, I32 maxi = 100) -> I32 {
    return RandGen::instance().uniform_int<I32>(mini, maxi);
}

inline auto gen_i32(const Vec2i& range) -> I32 {
    return RandGen::instance().uniform_int<I32>(range.x(), range.y());
}

} // namespace nv

#endif