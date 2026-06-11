#include <nvk/hla/HlaSubscriber.h>
#if NV_USE_HLA

#include <nvk/hla/HlaSubscriberAmbassador.h>
#include <nvk/log/LogManager.h>

// All OpenRTI includes are local to this .cpp — none leak into the public
// header. Include root is .../OpenRTI/include/rti1516e/ so <RTI/xxx.h> resolves
// correctly.
#include <RTI/RTI1516.h> // all-in-one: Handle, Typedefs, Enums, Ambassadors
#include <RTI/RTIambassadorFactory.h>
#include <RTI/time/HLAfloat64Time.h>

#include <chrono>

namespace nv {

// =============================================================================
// RtiImpl — holds RTI ambassador + cached handles.
// Defined here so RTI types never appear in HlaSubscriber.h.
// =============================================================================

struct HlaSubscriber::RtiImpl {
    std::unique_ptr<rti1516e::RTIambassador> rti;

    rti1516e::ObjectClassHandle classBaseEntity;
    rti1516e::AttributeHandle attrWorldLocation;
    rti1516e::AttributeHandle attrOrientation;
    rti1516e::AttributeHandle attrEntityType;
    rti1516e::AttributeHandle attrEntityId;
};

// =============================================================================
// Construction / destruction
// =============================================================================

HlaSubscriber::HlaSubscriber()
    : _ambassador(std::make_unique<HlaSubscriberAmbassador>()) {}

HlaSubscriber::~HlaSubscriber() { disconnect(); }

// =============================================================================
// connect()
// =============================================================================

auto HlaSubscriber::connect(const HlaFederateConfig& cfg) -> bool {
    if (_connected) {
        logWARN("HlaSubscriber::connect() called while already connected.");
        return false;
    }
    _config = cfg;
    _impl = std::make_unique<RtiImpl>();

    // 1. Create RTI ambassador
    try {
        rti1516e::RTIambassadorFactory factory;
        _impl->rti = factory.createRTIambassador();
    } catch (const rti1516e::RTIinternalError& e) {
        logERROR("HlaSubscriber: failed to create RTI ambassador: {}",
                 WStringToString(e.what()));
        _impl.reset();
        return false;
    }

    // 2. Connect to CRC
    try {
        _impl->rti->connect(*_ambassador, rti1516e::HLA_EVOKED,
                            to_wstr(cfg.rtiAddress));
    } catch (const rti1516e::ConnectionFailed& e) {
        logERROR("HlaSubscriber: RTI connection failed ({}): {}",
                 cfg.rtiAddress, WStringToString(e.what()));
        _impl.reset();
        return false;
    } catch (const rti1516e::Exception& e) {
        logERROR("HlaSubscriber: connect() exception: {}",
                 WStringToString(e.what()));
        _impl.reset();
        return false;
    }

    // 3. Create or join federation
    std::vector<std::wstring> fomModules;
    fomModules.reserve(cfg.fomPaths.size());
    for (const auto& p : cfg.fomPaths)
        fomModules.push_back(to_fom_uri(p));

    const std::wstring wFedName = to_wstr(cfg.federationName);
    const std::wstring wFedType = to_wstr(cfg.federateName);
    const std::wstring wTimeImpl = to_wstr(cfg.logicalTimeImpl);

    try {
        _impl->rti->createFederationExecution(wFedName, fomModules, wTimeImpl);
        logINFO("HlaSubscriber: federation '{}' created.", cfg.federationName);
    } catch (const rti1516e::FederationExecutionAlreadyExists&) {
        logINFO("HlaSubscriber: federation '{}' already exists, joining.",
                cfg.federationName);
    } catch (const rti1516e::Exception& e) {
        logERROR("HlaSubscriber: createFederationExecution failed: {}",
                 WStringToString(e.what()));
        try {
            _impl->rti->disconnect();
        } catch (...) {
        }
        _impl.reset();
        return false;
    }

    try {
        _impl->rti->joinFederationExecution(wFedType, wFedName);
        logINFO("HlaSubscriber: joined federation '{}' as '{}'.",
                cfg.federationName, cfg.federateName);
    } catch (const rti1516e::Exception& e) {
        logERROR("HlaSubscriber: joinFederationExecution failed: {}",
                 WStringToString(e.what()));
        try {
            _impl->rti->disconnect();
        } catch (...) {
        }
        _impl.reset();
        return false;
    }

    // 4. Look up handles and subscribe
    if (!lookup_handles()) {
        logERROR("HlaSubscriber: handle lookup failed — check FOM paths.");
        try {
            _impl->rti->resignFederationExecution(rti1516e::NO_ACTION);
        } catch (...) {
        }
        try {
            _impl->rti->disconnect();
        } catch (...) {
        }
        _impl.reset();
        return false;
    }
    subscribe_entity_classes();

    // 5. Enable time-constrained and wait for RTI grant
    try {
        _impl->rti->enableTimeConstrained();
    } catch (const rti1516e::Exception& e) {
        logERROR("HlaSubscriber: enableTimeConstrained failed: {}",
                 WStringToString(e.what()));
        try {
            _impl->rti->resignFederationExecution(rti1516e::NO_ACTION);
        } catch (...) {
        }
        try {
            _impl->rti->disconnect();
        } catch (...) {
        }
        _impl.reset();
        return false;
    }

    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!_ambassador->timeConstrainedReady.load(std::memory_order_acquire)) {
        try {
            _impl->rti->evokeMultipleCallbacks(0.0, 0.1);
        } catch (const rti1516e::Exception& e) {
            logERROR(
                "HlaSubscriber: evokeMultipleCallbacks during TC handshake: {}",
                WStringToString(e.what()));
            break;
        }
        if (std::chrono::steady_clock::now() > deadline) {
            logERROR(
                "HlaSubscriber: timed out waiting for time-constrained grant.");
            try {
                _impl->rti->resignFederationExecution(rti1516e::NO_ACTION);
            } catch (...) {
            }
            try {
                _impl->rti->disconnect();
            } catch (...) {
            }
            _impl.reset();
            return false;
        }
    }

    _connected = true;
    onConnected.emit();
    logINFO("HlaSubscriber: ready. Sim time = {:.3f}s",
            _ambassador->simTime.load());
    return true;
}

// =============================================================================
// disconnect()
// =============================================================================

void HlaSubscriber::disconnect() {
    if (!_impl)
        return;

    if (_impl->rti) {
        try {
            _impl->rti->resignFederationExecution(rti1516e::NO_ACTION);
        } catch (const rti1516e::Exception& e) {
            logWARN("HlaSubscriber: resign exception: {}",
                    WStringToString(e.what()));
        }
        try {
            _impl->rti->destroyFederationExecution(
                to_wstr(_config.federationName));
        } catch (const rti1516e::FederatesCurrentlyJoined&) {
            // expected — other federates still present
        } catch (...) {
        }
        try {
            _impl->rti->disconnect();
        } catch (...) {
        }
    }

    _impl.reset();
    _entities.clear();
    _simTime = 0.0;

    const bool wasConnected = _connected;
    _connected = false;
    if (wasConnected)
        onDisconnected.emit();
}

auto HlaSubscriber::is_connected() const noexcept -> bool { return _connected; }

// =============================================================================
// tick()
// =============================================================================

void HlaSubscriber::tick(F64 maxSeconds) {
    if (!_connected || !_impl || !_impl->rti)
        return;

    try {
        _impl->rti->evokeMultipleCallbacks(0.0, maxSeconds);
    } catch (const rti1516e::Exception& e) {
        logERROR("HlaSubscriber::tick: evokeMultipleCallbacks: {}",
                 WStringToString(e.what()));
        return;
    }

    static thread_local std::vector<HlaSubscriberAmbassador::EntityEvent>
        events;
    events.clear();
    _ambassador->drain_events(events);

    for (auto& ev : events) {
        switch (ev.kind) {
        case HlaSubscriberAmbassador::EventKind::Added:
            _entities[ev.entity.handle] = ev.entity;
            onEntityAdded.emit(ev.entity);
            break;
        case HlaSubscriberAmbassador::EventKind::Updated:
            _entities[ev.entity.handle] = ev.entity;
            onEntityUpdated.emit(ev.entity);
            break;
        case HlaSubscriberAmbassador::EventKind::Removed:
            _entities.erase(ev.removedHandle);
            onEntityRemoved.emit(ev.removedHandle);
            break;
        }
    }

    _simTime = _ambassador->simTime.load(std::memory_order_relaxed);
}

// =============================================================================
// Query
// =============================================================================

auto HlaSubscriber::entities() const noexcept
    -> const UnorderedMap<U64, SimEntity>& {
    return _entities;
}

auto HlaSubscriber::sim_time() const noexcept -> F64 { return _simTime; }

// =============================================================================
// Private helpers
// =============================================================================

auto HlaSubscriber::to_wstr(const String& s) -> std::wstring {
    return std::wstring(s.begin(), s.end());
}

auto HlaSubscriber::to_fom_uri(const String& path) -> std::wstring {
    if (path.rfind("file://", 0) == 0 || path.rfind("http://", 0) == 0)
        return to_wstr(path);
#if defined(_WIN32)
    String uri = "file:///" + path;
    for (char& c : uri)
        if (c == '\\')
            c = '/';
    return to_wstr(uri);
#else
    if (!path.empty() && path[0] == '/')
        return to_wstr("file://" + path);
    return to_wstr(path);
#endif
}

auto HlaSubscriber::lookup_handles() -> bool {
    try {
        _impl->classBaseEntity =
            _impl->rti->getObjectClassHandle(L"BaseEntity");
        _impl->attrWorldLocation = _impl->rti->getAttributeHandle(
            _impl->classBaseEntity, L"WorldLocation");
        _impl->attrOrientation = _impl->rti->getAttributeHandle(
            _impl->classBaseEntity, L"Orientation");
        _impl->attrEntityType = _impl->rti->getAttributeHandle(
            _impl->classBaseEntity, L"EntityType");
        _impl->attrEntityId = _impl->rti->getAttributeHandle(
            _impl->classBaseEntity, L"EntityIdentifier");
    } catch (const rti1516e::NameNotFound& e) {
        logERROR("HlaSubscriber: name not found in FOM: {}",
                 WStringToString(e.what()));
        return false;
    } catch (const rti1516e::Exception& e) {
        logERROR("HlaSubscriber: handle lookup exception: {}",
                 WStringToString(e.what()));
        return false;
    }

    _ambassador->init(_impl->classBaseEntity, _impl->attrWorldLocation,
                      _impl->attrOrientation, _impl->attrEntityType,
                      _impl->attrEntityId);

    logINFO("HlaSubscriber: attribute handles resolved.");
    return true;
}

void HlaSubscriber::subscribe_entity_classes() {
    rti1516e::AttributeHandleSet attrs;
    attrs.insert(_impl->attrWorldLocation);
    attrs.insert(_impl->attrOrientation);
    attrs.insert(_impl->attrEntityType);
    attrs.insert(_impl->attrEntityId);

    try {
        _impl->rti->subscribeObjectClassAttributes(_impl->classBaseEntity,
                                                   attrs, true);
        logINFO("HlaSubscriber: subscribed to BaseEntity attributes.");
    } catch (const rti1516e::Exception& e) {
        logERROR("HlaSubscriber: subscribeObjectClassAttributes failed: {}",
                 WStringToString(e.what()));
    }
}

} // namespace nv

#endif // NV_USE_HLA
