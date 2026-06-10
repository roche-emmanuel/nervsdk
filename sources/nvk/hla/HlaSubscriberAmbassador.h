#pragma once
#if NV_USE_HLA

#include <nvk/sim/SimEntity.h>
#include <nvk/base/SpinLock.h>
#include <nvk_types.h>

// OpenRTI 1516e — include root is .../OpenRTI/include/rti1516e/
// so includes are just <RTI/xxx.h> with no additional prefix.
#include <RTI/RTI1516.h>             // pulls in Handle.h, Typedefs.h, Enums.h, etc.
#include <RTI/NullFederateAmbassador.h>
#include <RTI/time/HLAfloat64Time.h>

#include <atomic>
#include <deque>
#include <functional>   // std::hash
#include <vector>

// rti1516e::ObjectInstanceHandle has a .hash() method but no std::hash<>
// specialisation. Provide one so we can use it as an UnorderedMap key.
namespace std {
    template<>
    struct hash<rti1516e::ObjectInstanceHandle> {
        size_t operator()(const rti1516e::ObjectInstanceHandle& h) const noexcept {
            return static_cast<size_t>(h.hash());
        }
    };
}

namespace nv {

/// Internal bridge between OpenRTI callbacks and the HlaSubscriber data model.
/// Implementation detail of HlaSubscriber — do not use directly.
///
/// Threading: HLA_EVOKED mode fires all callbacks on the main thread from
/// evokeMultipleCallbacks(). The SpinLock + staging deque are retained for
/// future HLA_IMMEDIATE upgrade safety.
class HlaSubscriberAmbassador : public rti1516e::NullFederateAmbassador {
public:

    enum class EventKind : U8 { Added, Updated, Removed };

    struct EntityEvent {
        EventKind kind           = EventKind::Added;
        SimEntity entity;
        U64       removedHandle  = 0;
    };

    /// Called by HlaSubscriber after joinFederationExecution() to cache handles.
    void init(rti1516e::ObjectClassHandle baseEntityClass,
              rti1516e::AttributeHandle   worldLocationAttr,
              rti1516e::AttributeHandle   orientationAttr,
              rti1516e::AttributeHandle   entityTypeAttr,
              rti1516e::AttributeHandle   entityIdAttr) noexcept;

    /// Move all staged events into @p out and clear the buffer. Thread-safe.
    void drain_events(std::vector<EntityEvent>& out);

    /// Current sim logical time in seconds (atomic, safe to read from main thread).
    std::atomic<F64>  simTime{0.0};

    /// Set true by timeConstrainedEnabled(); polled by HlaSubscriber::connect().
    std::atomic<bool> timeConstrainedReady{false};

protected:
    // -------------------------------------------------------------------------
    // rti1516e::NullFederateAmbassador overrides

    // 6.9
    void discoverObjectInstance(
        rti1516e::ObjectInstanceHandle  theObject,
        rti1516e::ObjectClassHandle     theObjectClass,
        std::wstring const &            theObjectInstanceName)
        RTI_THROW ((rti1516e::FederateInternalError)) override;

    // 6.11 — timestamped (TSO delivery — most common)
    void reflectAttributeValues(
        rti1516e::ObjectInstanceHandle           theObject,
        rti1516e::AttributeHandleValueMap const& theAttributeValues,
        rti1516e::VariableLengthData const&      theUserSuppliedTag,
        rti1516e::OrderType                      sentOrder,
        rti1516e::TransportationType             theType,
        rti1516e::LogicalTime const&             theTime,
        rti1516e::OrderType                      receivedOrder,
        rti1516e::MessageRetractionHandle        theHandle,
        rti1516e::SupplementalReflectInfo        theReflectInfo)
        RTI_THROW ((rti1516e::FederateInternalError)) override;

    // 6.11 — receive-order (RO delivery — no timestamp)
    void reflectAttributeValues(
        rti1516e::ObjectInstanceHandle           theObject,
        rti1516e::AttributeHandleValueMap const& theAttributeValues,
        rti1516e::VariableLengthData const&      theUserSuppliedTag,
        rti1516e::OrderType                      sentOrder,
        rti1516e::TransportationType             theType,
        rti1516e::SupplementalReflectInfo        theReflectInfo)
        RTI_THROW ((rti1516e::FederateInternalError)) override;

    // 6.15 — 4-param overload (no time, no retraction handle)
    void removeObjectInstance(
        rti1516e::ObjectInstanceHandle      theObject,
        rti1516e::VariableLengthData const& theUserSuppliedTag,
        rti1516e::OrderType                 sentOrder,
        rti1516e::SupplementalRemoveInfo    theRemoveInfo)
        RTI_THROW ((rti1516e::FederateInternalError)) override;

    // 6.15 — 6-param overload (timestamped, no retraction handle)
    void removeObjectInstance(
        rti1516e::ObjectInstanceHandle      theObject,
        rti1516e::VariableLengthData const& theUserSuppliedTag,
        rti1516e::OrderType                 sentOrder,
        rti1516e::LogicalTime const&        theTime,
        rti1516e::OrderType                 receivedOrder,
        rti1516e::SupplementalRemoveInfo    theRemoveInfo)
        RTI_THROW ((rti1516e::FederateInternalError)) override;

    // 6.15 — 7-param overload (timestamped + retraction handle)
    void removeObjectInstance(
        rti1516e::ObjectInstanceHandle      theObject,
        rti1516e::VariableLengthData const& theUserSuppliedTag,
        rti1516e::OrderType                 sentOrder,
        rti1516e::LogicalTime const&        theTime,
        rti1516e::OrderType                 receivedOrder,
        rti1516e::MessageRetractionHandle   theHandle,
        rti1516e::SupplementalRemoveInfo    theRemoveInfo)
        RTI_THROW ((rti1516e::FederateInternalError)) override;

    // 8.6
    void timeConstrainedEnabled(
        rti1516e::LogicalTime const& theFederateTime)
        RTI_THROW ((rti1516e::FederateInternalError)) override;

private:
    // Cached handles — set once by init(), then read-only
    rti1516e::ObjectClassHandle _baseEntityClass;
    rti1516e::AttributeHandle   _attrWorldLocation;
    rti1516e::AttributeHandle   _attrOrientation;
    rti1516e::AttributeHandle   _attrEntityType;
    rti1516e::AttributeHandle   _attrEntityId;
    bool                        _handlesReady = false;

    // Per-instance partial state (EVOKED = main thread only, no lock needed here)
    std::unordered_map<rti1516e::ObjectInstanceHandle, SimEntity> _partialEntities;

    // Staging buffer
    SpinLock                _eventLock;
    std::deque<EntityEvent> _stagedEvents;

    void stage_event(EntityEvent ev);
    void do_remove(rti1516e::ObjectInstanceHandle const& theObject);

    bool apply_attribute(
        SimEntity&                          entity,
        rti1516e::AttributeHandle const&    handle,
        rti1516e::VariableLengthData const& value,
        F64                                 simTimeSecs);

    static auto to_handle_key(
        rti1516e::ObjectInstanceHandle const& h) noexcept -> U64;
};

} // namespace nv

#endif // NV_USE_HLA
