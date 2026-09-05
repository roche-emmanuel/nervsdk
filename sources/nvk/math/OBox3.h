#ifndef _NV_OBOX3_H_
#define _NV_OBOX3_H_

#include <nvk/math/Box3.h>
#include <nvk/math/Mat3.h>
#include <nvk/math/Quat.h>
#include <nvk/math/Vec3.h>

namespace nv {

/**
 * OBox3: an oriented (rotated) 3D box.
 *
 * Stored as a world-space center, an orientation (the columns of `rotation`
 * are the box's local X/Y/Z axes, expressed in world space) and the
 * half-extent along each of those local axes — rather than as a Box3 plus a
 * separate transform, so every query (corners, AABB, overlap) reads
 * directly off the three members with no re-derivation.
 *
 * Typical use: a placed object's local (object-space) bounding box, known
 * as a Box3, needs to be checked against other placed objects once it has
 * been positioned and rotated into the world — vehicles on a road lane,
 * aircraft parked on a stand, anything dropped into a scene at an
 * orientation. from_local_box() does that transform once; intersects()
 * then answers "do these two placed boxes overlap" without either box ever
 * being collapsed to axis-aligned (which would report overlaps that don't
 * really exist for two boxes meeting at an angle).
 */
template <typename T> struct OBox3 {
    using value_t = T;

    /** Center of the box, in world space. */
    Vec3<T> center;

    /** Orientation: column i is the box's local axis i, in world space. */
    Mat3<T> rotation;

    /** Half-extent along each local axis. */
    Vec3<T> halfExtent;

    OBox3() = default;

    OBox3(const Vec3<T>& center_, const Mat3<T>& rotation_,
         const Vec3<T>& halfExtent_)
        : center(center_), rotation(rotation_), halfExtent(halfExtent_) {}

    /**
     * Builds the world-space oriented box for a local-space box that has
     * been placed at `position` with orientation `rotation`.
     *
     * The local box's own center offset (it need not be centered on the
     * object's pivot) is folded into the resulting world center, so callers
     * can pass a raw mesh/object bounding box straight through.
     */
    static auto from_local_box(const Box3<T>& localBox, const Vec3<T>& position,
                               const Mat3<T>& rotation_) -> OBox3<T> {
        OBox3<T> obox;
        obox.rotation = rotation_;
        obox.center = position + rotation_ * localBox.center();
        obox.halfExtent = Vec3<T>((localBox.xmax - localBox.xmin) * T(0.5),
                                  (localBox.ymax - localBox.ymin) * T(0.5),
                                  (localBox.zmax - localBox.zmin) * T(0.5));
        return obox;
    }

    static auto from_local_box(const Box3<T>& localBox, const Vec3<T>& position,
                               const Quaternion<T>& orientation) -> OBox3<T> {
        return from_local_box(localBox, position, Mat3<T>(orientation));
    }

    /** Local axis i (0=X, 1=Y, 2=Z), in world space. Unit length as long as
        `rotation` is a proper rotation matrix. */
    [[nodiscard]] auto axis(U32 i) const -> Vec3<T> { return rotation.col(i); }

    [[nodiscard]] auto axisX() const -> Vec3<T> { return rotation.col(0); }
    [[nodiscard]] auto axisY() const -> Vec3<T> { return rotation.col(1); }
    [[nodiscard]] auto axisZ() const -> Vec3<T> { return rotation.col(2); }

    /** World-space position of corner `index` (0..7): bit 0 selects the
        +/-X face, bit 1 the +/-Y face, bit 2 the +/-Z face. */
    [[nodiscard]] auto corner(U32 index) const -> Vec3<T> {
        const T sx = (index & 1U) ? T(1) : T(-1);
        const T sy = (index & 2U) ? T(1) : T(-1);
        const T sz = (index & 4U) ? T(1) : T(-1);
        return center + axisX() * (sx * halfExtent.x()) +
               axisY() * (sy * halfExtent.y()) + axisZ() * (sz * halfExtent.z());
    }

    /** True when `point` (world space) falls inside this box. */
    [[nodiscard]] auto contains(const Vec3<T>& point) const -> bool {
        const Vec3<T> d = point - center;
        return std::abs(d.dot(axisX())) <= halfExtent.x() &&
              std::abs(d.dot(axisY())) <= halfExtent.y() &&
              std::abs(d.dot(axisZ())) <= halfExtent.z();
    }

    /** The world-space axis-aligned bound of this oriented box. Built from
        all 8 corners rather than approximated from the half-extents, since
        the box's axes are not generally aligned with world X/Y/Z. */
    [[nodiscard]] auto aabb() const -> Box3<T> {
        Box3<T> box(center);
        for (U32 i = 0; i < 8; ++i) {
            box.extendTo(corner(i));
        }
        return box;
    }

    /**
     * True when this box overlaps `other`, via the separating axis theorem:
     * the 3 face normals of each box, plus the 9 pairwise cross products of
     * their edge directions. If any of those 15 axes separates the boxes
     * they are guaranteed disjoint; if none does, they overlap.
     *
     * Ericson, "Real-Time Collision Detection", section 4.4.1.
     */
    [[nodiscard]] auto intersects(const OBox3<T>& other) const -> bool {
        const Vec3<T> axesA[3] = {axisX(), axisY(), axisZ()};
        const Vec3<T> axesB[3] = {other.axisX(), other.axisY(), other.axisZ()};
        const T halfA[3] = {halfExtent.x(), halfExtent.y(), halfExtent.z()};
        const T halfB[3] = {other.halfExtent.x(), other.halfExtent.y(),
                            other.halfExtent.z()};

        // rot[i][j] expresses B's axis j in A's frame; absRot adds a small
        // epsilon (R entries are dot products of unit vectors, so always in
        // [-1, 1] regardless of scale) so near-parallel edges don't
        // spuriously pass the cross-product axes due to floating point
        // noise.
        T rot[3][3];
        T absRot[3][3];
        for (I32 i = 0; i < 3; ++i) {
            for (I32 j = 0; j < 3; ++j) {
                rot[i][j] = axesA[i].dot(axesB[j]);
                absRot[i][j] = std::abs(rot[i][j]) + T(1e-6);
            }
        }

        const Vec3<T> centerDelta = other.center - center;
        const T t[3] = {centerDelta.dot(axesA[0]), centerDelta.dot(axesA[1]),
                        centerDelta.dot(axesA[2])};

        // L = A's own face normals.
        for (I32 i = 0; i < 3; ++i) {
            const T extent = halfA[i] + halfB[0] * absRot[i][0] +
                             halfB[1] * absRot[i][1] + halfB[2] * absRot[i][2];
            if (std::abs(t[i]) > extent) {
                return false;
            }
        }

        // L = B's own face normals.
        for (I32 j = 0; j < 3; ++j) {
            const T extent = halfB[j] + halfA[0] * absRot[0][j] +
                             halfA[1] * absRot[1][j] + halfA[2] * absRot[2][j];
            const T proj = t[0] * rot[0][j] + t[1] * rot[1][j] + t[2] * rot[2][j];
            if (std::abs(proj) > extent) {
                return false;
            }
        }

        // L = cross(A_i, B_j), the 9 edge/edge cases.
        for (I32 i = 0; i < 3; ++i) {
            const I32 i1 = (i + 1) % 3;
            const I32 i2 = (i + 2) % 3;
            for (I32 j = 0; j < 3; ++j) {
                const I32 j1 = (j + 1) % 3;
                const I32 j2 = (j + 2) % 3;

                const T extent = halfA[i1] * absRot[i2][j] +
                                 halfA[i2] * absRot[i1][j] +
                                 halfB[j1] * absRot[i][j2] +
                                 halfB[j2] * absRot[i][j1];
                const T proj = t[i2] * rot[i1][j] - t[i1] * rot[i2][j];
                if (std::abs(proj) > extent) {
                    return false;
                }
            }
        }

        return true;
    }
};

using OBox3f = OBox3<F32>;
using OBox3d = OBox3<F64>;

} // namespace nv

namespace std {
inline auto operator<<(std::ostream& os, const nv::OBox3f& box)
    -> std::ostream& {
    os << "nv::OBox3f(center=" << box.center << ", halfExtent="
       << box.halfExtent << ")";
    return os;
}
inline auto operator<<(std::ostream& os, const nv::OBox3d& box)
    -> std::ostream& {
    os << "nv::OBox3d(center=" << box.center << ", halfExtent="
       << box.halfExtent << ")";
    return os;
}
} // namespace std

#endif
