#ifndef _NV_UE_FRAME_H_
#define _NV_UE_FRAME_H_

#include <nvk/math/Quat.h>

/** Conversions between the native NervSDK orientation frame (WebGPU
 * convention: X=right, Y=up, Z=forward) and the Unreal Engine actor frame
 * (X=forward, Y=right, Z=up).
 *
 * Both frames are left-handed and label their axes with the same semantics,
 * so the change of basis is a cyclic permutation of components with
 * determinant +1: a proper rotation. Consequences:
 *   - vectors are permuted componentwise,
 *   - quaternions are transported by permuting the vector part only, leaving
 *     the scalar part untouched: no angle or sign change is involved.
 *
 * This header deliberately depends on no engine type: the UE-side values are
 * plain nv::Vec3/nv::Quaternion holding UE-frame components, ready to be fed
 * to FVector / FQuat / FRotator.
 *
 */
namespace nv {

/** Convert a position expressed in the UE frame to the native frame.
    In UE what we call "X" is what we call "Z" in nv,
    In UE what we call "Y" is what we call "X" in nv,
    In UE what we call "Z" is what we call "Y" in nv,
*/
template <typename T>
[[nodiscard]] inline auto fromUE(const Vec3<T>& vec) -> Vec3<T> {
    return {vec.y(), vec.z(), vec.x()};
}

/** Convert a position expressed in the native frame to the UE frame.
    In NV what we call "X" is what we call "Y" in UE,
    In NV what we call "Y" is what we call "Z" in UE,
    In NV what we call "Z" is what we call "X" in UE,
*/
template <typename T>
[[nodiscard]] inline auto toUE(const Vec3<T>& vec) -> Vec3<T> {
    return {vec.z(), vec.x(), vec.y()};
}

/** Convert a rotation expressed in the UE frame (ie. FQuat components in
 * X,Y,Z,W order) to the native frame. */
template <typename T>
[[nodiscard]] inline auto fromUE(const Quaternion<T>& quat) -> Quaternion<T> {
    return {quat.y(), quat.z(), quat.x(), quat.w()};
}

/** Convert a native-frame rotation to UE-frame components. The result is a
 * Quaternion object holding FQuat components, not a native rotation: only
 * hand it to FQuat, never multiply it with native quaternions. */
template <typename T>
[[nodiscard]] inline auto toUE(const Quaternion<T>& quat) -> Quaternion<T> {
    return {quat.z(), quat.x(), quat.y(), quat.w()};
}

/** Build a native rotation from UE FRotator angles, in degrees.
 * UE and NervSDK share the same intrinsic yaw/pitch/roll ordering, so only
 * the yaw and roll senses differ. */
template <typename T>
[[nodiscard]] inline auto from_rotator_angles(T pitch, T yaw, T roll)
    -> Quaternion<T> {
    return Quaternion<T>::from_ypr(-yaw, pitch, -roll);
}

/** Return the UE FRotator angles of a native rotation, in degrees, packed as
 * (pitch, yaw, roll) to match the FRotator constructor order. */
template <typename T>
[[nodiscard]] inline auto to_rotator_angles(const Quaternion<T>& quat)
    -> Vec3<T> {
    auto ypr = quat.to_ypr();
    return {ypr.y(), -ypr.x(), -ypr.z()};
}

/** Build a UE-frame rotation quaternion from forward/up axes expressed in the
 * UE frame. Equivalent to converting both axes to the native frame, calling
 * Quaternion::from_axes(), and converting the result back to UE — this just
 * names that round trip so call sites don't have to spell it out.
 *
 * The inputs need not be normalized nor orthogonal: 'ueFwd' is honoured
 * exactly, and 'ueUp' is only used as a hint to resolve the roll around it.
 * The result is a Quaternion object holding FQuat components: only hand it
 * to FQuat, never multiply it with native quaternions. */
template <typename T>
[[nodiscard]] inline auto unreal_from_axes(const Vec3<T>& ueFwd,
                                           const Vec3<T>& ueUp)
    -> Quaternion<T> {
    return toUE(Quaternion<T>::from_axes(fromUE(ueFwd), fromUE(ueUp)));
}

} // namespace nv

#endif