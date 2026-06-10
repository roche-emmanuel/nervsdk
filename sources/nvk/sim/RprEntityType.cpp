#include <nvk/sim/RprEntityType.h>
#include <nvk_common.h> // fmt / string formatting

namespace nv {

auto RprEntityType::to_string() const -> String {
    return fmt::format("{}.{}.{}.{}.{}.{}.{}", (int)kind, (int)domain,
                       (int)countryCode, (int)category, (int)subcategory,
                       (int)specific, (int)extra);
}

auto RprEntityType::kind_name() const -> const char* {
    switch (kind) {
    case 0:
        return "Other";
    case 1:
        return "Platform";
    case 2:
        return "Munition";
    case 3:
        return "LifeForm";
    case 4:
        return "Environmental";
    case 5:
        return "CulturalFeature";
    case 6:
        return "Supply";
    case 7:
        return "Radio";
    case 8:
        return "Expendable";
    case 9:
        return "SensorEmitter";
    default:
        return "Unknown";
    }
}

auto RprEntityType::domain_name() const -> const char* {
    // Domain semantics are well-defined for Platform (kind==1); other kinds
    // reuse the same numeric codes but with different interpretations.
    switch (domain) {
    case 0:
        return "Other";
    case 1:
        return "Land";
    case 2:
        return "Air";
    case 3:
        return "Surface";
    case 4:
        return "Subsurface";
    case 5:
        return "Space";
    default:
        return "Unknown";
    }
}

} // namespace nv
