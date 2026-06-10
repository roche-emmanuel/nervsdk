#include <nvk/hla/HlaSubscriberAmbassador.h>
#if NV_USE_HLA

#include <nvk/sim/RprDecoder.h>
#include <nvk/log/LogManager.h>

namespace nv {

// =============================================================================
// Setup
// =============================================================================

void HlaSubscriberAmbassador::init(
    rti1516e::ObjectClassHandle baseEntityClass,
    rti1516e::AttributeHandle   worldLocationAttr,
    rti1516e::AttributeHandle   orientationAttr,
    rti1516e::AttributeHandle   entityTypeAttr,
    rti1516e::AttributeHandle   entityIdAttr) noexcept
{
    _baseEntityClass    = baseEntityClass;
    _attrWorldLocation  = worldLocationAttr;
    _attrOrientation    = orientationAttr;
    _attrEntityType     = entityTypeAttr;
    _attrEntityId       = entityIdAttr;
    _handlesReady       = true;
}

// =============================================================================
// Main-thread interface
// =============================================================================

void HlaSubscriberAmbassador::drain_events(std::vector<EntityEvent>& out)
{
    SpinLockGuard guard(_eventLock);
    out.insert(out.end(),
               std::make_move_iterator(_stagedEvents.begin()),
               std::make_move_iterator(_stagedEvents.end()));
    _stagedEvents.clear();
}

// =============================================================================
// RTI callbacks — object lifecycle
// =============================================================================

void HlaSubscriberAmbassador::discoverObjectInstance(
    rti1516e::ObjectInstanceHandle  theObject,
    rti1516e::ObjectClassHandle     /*theObjectClass*/,
    std::wstring const &            theObjectInstanceName)
    RTI_THROW ((rti1516e::FederateInternalError))
{
    if (!_handlesReady)
        return;

    const U64 key = to_handle_key(theObject);

    SimEntity& entity = _partialEntities[theObject];
    entity.handle     = key;
    entity.name       = std::string(theObjectInstanceName.begin(),
                                    theObjectInstanceName.end());

    EntityEvent ev;
    ev.kind   = EventKind::Added;
    ev.entity = entity;
    stage_event(std::move(ev));
}

// =============================================================================
// RTI callbacks — attribute updates
// =============================================================================

// Timestamped (TSO) delivery
void HlaSubscriberAmbassador::reflectAttributeValues(
    rti1516e::ObjectInstanceHandle           theObject,
    rti1516e::AttributeHandleValueMap const& theAttributeValues,
    rti1516e::VariableLengthData const&      /*theUserSuppliedTag*/,
    rti1516e::OrderType                      /*sentOrder*/,
    rti1516e::TransportationType             /*theType*/,
    rti1516e::LogicalTime const&             theTime,
    rti1516e::OrderType                      /*receivedOrder*/,
    rti1516e::MessageRetractionHandle        /*theHandle*/,
    rti1516e::SupplementalReflectInfo        /*theReflectInfo*/)
    RTI_THROW ((rti1516e::FederateInternalError))
{
    if (!_handlesReady)
        return;

    auto it = _partialEntities.find(theObject);
    if (it == _partialEntities.end()) {
        logWARN("HlaSubscriberAmbassador: TSO reflect for unknown instance.");
        return;
    }

    F64 simTimeSecs = 0.0;
    try {
        simTimeSecs = dynamic_cast<const rti1516e::HLAfloat64Time&>(theTime).getTime();
        simTime.store(simTimeSecs, std::memory_order_relaxed);
    } catch (const std::bad_cast&) {
        simTimeSecs = simTime.load(std::memory_order_relaxed);
    }

    SimEntity& entity = it->second;
    bool changed = false;
    for (auto const& [handle, value] : theAttributeValues)
        changed |= apply_attribute(entity, handle, value, simTimeSecs);

    if (changed) {
        EntityEvent ev;
        ev.kind   = EventKind::Updated;
        ev.entity = entity;
        stage_event(std::move(ev));
    }
}

// Receive-order (RO) delivery — no timestamp
void HlaSubscriberAmbassador::reflectAttributeValues(
    rti1516e::ObjectInstanceHandle           theObject,
    rti1516e::AttributeHandleValueMap const& theAttributeValues,
    rti1516e::VariableLengthData const&      /*theUserSuppliedTag*/,
    rti1516e::OrderType                      /*sentOrder*/,
    rti1516e::TransportationType             /*theType*/,
    rti1516e::SupplementalReflectInfo        /*theReflectInfo*/)
    RTI_THROW ((rti1516e::FederateInternalError))
{
    if (!_handlesReady)
        return;

    auto it = _partialEntities.find(theObject);
    if (it == _partialEntities.end()) {
        logWARN("HlaSubscriberAmbassador: RO reflect for unknown instance.");
        return;
    }

    SimEntity& entity = it->second;
    const F64 simTimeSecs = simTime.load(std::memory_order_relaxed);
    bool changed = false;
    for (auto const& [handle, value] : theAttributeValues)
        changed |= apply_attribute(entity, handle, value, simTimeSecs);

    if (changed) {
        EntityEvent ev;
        ev.kind   = EventKind::Updated;
        ev.entity = entity;
        stage_event(std::move(ev));
    }
}

// =============================================================================
// RTI callbacks — object removal (all three overloads delegate to do_remove)
// =============================================================================

void HlaSubscriberAmbassador::removeObjectInstance(
    rti1516e::ObjectInstanceHandle      theObject,
    rti1516e::VariableLengthData const& /*theUserSuppliedTag*/,
    rti1516e::OrderType                 /*sentOrder*/,
    rti1516e::SupplementalRemoveInfo    /*theRemoveInfo*/)
    RTI_THROW ((rti1516e::FederateInternalError))
{
    do_remove(theObject);
}

void HlaSubscriberAmbassador::removeObjectInstance(
    rti1516e::ObjectInstanceHandle      theObject,
    rti1516e::VariableLengthData const& /*theUserSuppliedTag*/,
    rti1516e::OrderType                 /*sentOrder*/,
    rti1516e::LogicalTime const&        /*theTime*/,
    rti1516e::OrderType                 /*receivedOrder*/,
    rti1516e::SupplementalRemoveInfo    /*theRemoveInfo*/)
    RTI_THROW ((rti1516e::FederateInternalError))
{
    do_remove(theObject);
}

void HlaSubscriberAmbassador::removeObjectInstance(
    rti1516e::ObjectInstanceHandle      theObject,
    rti1516e::VariableLengthData const& /*theUserSuppliedTag*/,
    rti1516e::OrderType                 /*sentOrder*/,
    rti1516e::LogicalTime const&        /*theTime*/,
    rti1516e::OrderType                 /*receivedOrder*/,
    rti1516e::MessageRetractionHandle   /*theHandle*/,
    rti1516e::SupplementalRemoveInfo    /*theRemoveInfo*/)
    RTI_THROW ((rti1516e::FederateInternalError))
{
    do_remove(theObject);
}

// =============================================================================
// Time management
// =============================================================================

void HlaSubscriberAmbassador::timeConstrainedEnabled(
    rti1516e::LogicalTime const& theFederateTime)
    RTI_THROW ((rti1516e::FederateInternalError))
{
    try {
        const F64 t = dynamic_cast<const rti1516e::HLAfloat64Time&>(theFederateTime).getTime();
        simTime.store(t, std::memory_order_relaxed);
    } catch (const std::bad_cast&) {}

    timeConstrainedReady.store(true, std::memory_order_release);
    logINFO("HlaSubscriberAmbassador: time-constrained mode enabled.");
}

// =============================================================================
// Private helpers
// =============================================================================

void HlaSubscriberAmbassador::stage_event(EntityEvent ev)
{
    SpinLockGuard guard(_eventLock);
    _stagedEvents.push_back(std::move(ev));
}

void HlaSubscriberAmbassador::do_remove(rti1516e::ObjectInstanceHandle const& theObject)
{
    _partialEntities.erase(theObject);

    EntityEvent ev;
    ev.kind          = EventKind::Removed;
    ev.removedHandle = to_handle_key(theObject);
    stage_event(std::move(ev));
}

bool HlaSubscriberAmbassador::apply_attribute(
    SimEntity&                          entity,
    rti1516e::AttributeHandle const&    handle,
    rti1516e::VariableLengthData const& value,
    F64                                 simTimeSecs)
{
    const auto* buf = static_cast<const U8*>(value.data());
    const size_t len = value.size();

    if (handle == _attrWorldLocation && len >= 24) {
        entity.worldPos       = RprDecoder::decode_world_location(buf, len);
        entity.positionValid  = true;
        entity.lastUpdateTime = simTimeSecs;
        return true;
    }
    if (handle == _attrOrientation && len >= 12) {
        entity.orientation      = RprDecoder::decode_orientation(buf, len);
        entity.orientationValid = true;
        entity.lastUpdateTime   = simTimeSecs;
        return true;
    }
    if (handle == _attrEntityType && len >= 8) {
        entity.entityType     = RprDecoder::decode_entity_type(buf, len);
        entity.typeValid      = true;
        entity.lastUpdateTime = simTimeSecs;
        return true;
    }
    if (handle == _attrEntityId && len >= 6) {
        entity.handle         = RprDecoder::decode_entity_id(buf, len);
        entity.lastUpdateTime = simTimeSecs;
        return true;
    }
    return false;
}

auto HlaSubscriberAmbassador::to_handle_key(
    rti1516e::ObjectInstanceHandle const& h) noexcept -> U64
{
    // Use the RTI's own hash — stable within a session.
    // Will be overwritten by decode_entity_id() once EntityIdentifier arrives.
    return static_cast<U64>(h.hash());
}

} // namespace nv

#endif // NV_USE_HLA
