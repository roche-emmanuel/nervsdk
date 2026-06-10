#pragma once
#if NV_USE_HLA

#include <nvk_common.h>
#include <nvk_types.h>
#include <nvk/base/Signal.h>
#include <nvk/sim/SimEntity.h>
#include <nvk/hla/HlaFederateConfig.h>

// No OpenRTI headers here — clients of HlaSubscriber.h must not depend on RTI.
// All RTI types are hidden behind the Handles pimpl in the .cpp.

namespace nv {

class HlaSubscriberAmbassador;

/// Passive HLA 1516e subscriber.
///
/// Connects to an existing federation as a time-constrained (non-regulating)
/// federate and receives RPR-FOM 2.0 entity state updates. Never publishes.
///
/// Usage:
/// @code
///   nv::HlaSubscriber sub;
///   sub.onEntityAdded.connect([](const nv::SimEntity& e) { ... });
///   sub.onEntityUpdated.connect([](const nv::SimEntity& e) { ... });
///   sub.onEntityRemoved.connect([](nv::U64 handle) { ... });
///
///   nv::HlaFederateConfig cfg;
///   cfg.federationName = "MySim";
///   cfg.rtiAddress     = "crcAddress=sim-host:8989";
///   cfg.fomPaths       = { "RPR_FOM_v2.0_Base.xml" };
///   sub.connect(cfg);
///
///   // Each render frame:
///   sub.tick();
/// @endcode
class HlaSubscriber {
    NV_DECLARE_NO_COPY(HlaSubscriber)
public:
    HlaSubscriber();
    ~HlaSubscriber();

    // -------------------------------------------------------------------------
    // Lifecycle

    /// Connect to RTI, join federation, subscribe to all BaseEntity subclasses.
    /// Blocks briefly for the time-constrained handshake (≤ 5 s).
    /// @return true on success; false on any RTI error (details logged).
    auto connect(const HlaFederateConfig& cfg) -> bool;

    /// Resign from the federation and disconnect from RTI.
    /// Safe to call even if not connected.
    void disconnect();

    [[nodiscard]] auto is_connected() const noexcept -> bool;

    // -------------------------------------------------------------------------
    // Per-frame update

    /// Drain pending RTI callbacks and fire signals for state changes.
    /// Call once per render frame from the main thread.
    /// @param maxSeconds  Max wall-clock budget for RTI callback processing (default 1 ms).
    void tick(F64 maxSeconds = 0.001);

    // -------------------------------------------------------------------------
    // Query

    /// Read-only snapshot of all currently known entities.
    [[nodiscard]] auto entities() const noexcept
        -> const UnorderedMap<U64, SimEntity>&;

    /// Current simulation logical time in seconds.
    [[nodiscard]] auto sim_time() const noexcept -> F64;

    // -------------------------------------------------------------------------
    // Signals — fired from tick(), always on the main thread

    Signal<const SimEntity&> onEntityAdded;
    Signal<const SimEntity&> onEntityUpdated;
    Signal<U64>              onEntityRemoved;
    Signal<>                 onConnected;
    Signal<>                 onDisconnected;

private:
    // RTI handle types are hidden in this opaque struct, defined in the .cpp.
    // This keeps all OpenRTI headers out of HlaSubscriber.h entirely.
    struct Handles;

    static auto to_fom_uri(const String& path) -> std::wstring;
    static auto to_wstr(const String& s) -> std::wstring;
    auto lookup_handles() -> bool;
    void subscribe_entity_classes();

    // RTI objects — unique_ptr with forward-declared types (defined in .cpp)
    struct RtiImpl;
    std::unique_ptr<RtiImpl>                 _impl;   // owns RTIambassador + Handles
    std::unique_ptr<HlaSubscriberAmbassador> _ambassador;

    UnorderedMap<U64, SimEntity> _entities;
    F64                          _simTime   = 0.0;
    bool                         _connected = false;
    HlaFederateConfig            _config;
};

} // namespace nv

#endif // NV_USE_HLA
