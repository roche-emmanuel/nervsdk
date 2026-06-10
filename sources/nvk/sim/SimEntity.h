#pragma once

#include <nvk/math/Vec3.h>
#include <nvk/sim/RprEntityType.h>
#include <nvk_common.h>
#include <nvk_types.h>

namespace nv {

/// Snapshot of a simulated entity's observable state as received from an
/// HLA/RPR-FOM 2.0 federation (or any equivalent source).
///
/// Coordinate system for worldPos:
///   ECEF (Earth-Centred Earth-Fixed), metres.
///   X points toward (lat=0, lon=0), Y toward (lat=0, lon=90°E), Z toward North
///   Pole. Compatible directly with osgEarth GeoPoint in ECEF SRS.
///
/// Orientation convention (RPR-FOM 2.0 / DIS):
///   orientation = (psi, theta, phi) in radians.
///   psi   = heading / yaw   (rotation around body-Z, 0 = North, clockwise
///   positive) theta = pitch / elevation (rotation around body-Y) phi   = roll
///   (rotation around body-X) Applied in order: psi → theta → phi  (Tait-Bryan
///   ZYX intrinsic).
///
/// The validity flags indicate whether each field has been received at least
/// once from the RTI. Until the first update arrives, the corresponding field
/// holds its default zero value.
struct SimEntity {

    // -------------------------------------------------------------------------
    // Identity

    /// Opaque unique handle — maps to the RTI ObjectInstanceHandle value.
    /// Stable for the lifetime of the object in the federation.
    U64 handle = 0;

    /// Human-readable federation object name (assigned by the publishing
    /// federate).
    String name;

    // -------------------------------------------------------------------------
    // State

    /// Position in ECEF coordinates (metres).
    Vec3d worldPos = {};

    /// Orientation as (psi, theta, phi) in radians — RPR-FOM / DIS convention.
    Vec3f orientation = {};

    /// RPR-FOM EntityType (kind/domain/country/category/sub/specific/extra).
    RprEntityType entityType = {};

    // -------------------------------------------------------------------------
    // Bookkeeping

    /// Simulation logical time of the most recent attribute update (seconds).
    F64 lastUpdateTime = 0.0;

    /// True once a WorldLocation attribute update has been received.
    bool positionValid = false;

    /// True once an Orientation attribute update has been received.
    bool orientationValid = false;

    /// True once an EntityType attribute update has been received.
    bool typeValid = false;

    // -------------------------------------------------------------------------
    // Helpers

    /// True when all three core attributes have been received at least once.
    [[nodiscard]] auto is_fully_initialised() const noexcept -> bool {
        return positionValid && orientationValid && typeValid;
    }
};

} // namespace nv
