#include <nvk/pcg/overture_maps.h>

namespace nv {

static const std::unordered_map<std::string, LandUseClass> kLandUseMap = {
    {"aboriginal_land", LandUseClass::aboriginal_land},
    {"airfield", LandUseClass::airfield},
    {"allotments", LandUseClass::allotments},
    {"animal_keeping", LandUseClass::animal_keeping},
    {"aquaculture", LandUseClass::aquaculture},
    {"barracks", LandUseClass::barracks},
    {"base", LandUseClass::base},
    {"beach_resort", LandUseClass::beach_resort},
    {"brownfield", LandUseClass::brownfield},
    {"bunker", LandUseClass::bunker},
    {"camp_site", LandUseClass::camp_site},
    {"cemetery", LandUseClass::cemetery},
    {"clinic", LandUseClass::clinic},
    {"college", LandUseClass::college},
    {"commercial", LandUseClass::commercial},
    {"connection", LandUseClass::connection},
    {"construction", LandUseClass::construction},
    {"danger_area", LandUseClass::danger_area},
    {"doctors", LandUseClass::doctors},
    {"dog_park", LandUseClass::dog_park},
    {"downhill", LandUseClass::downhill},
    {"driving_range", LandUseClass::driving_range},
    {"driving_school", LandUseClass::driving_school},
    {"education", LandUseClass::education},
    {"environmental", LandUseClass::environmental},
    {"fairway", LandUseClass::fairway},
    {"farmland", LandUseClass::farmland},
    {"farmyard", LandUseClass::farmyard},
    {"fatbike", LandUseClass::fatbike},
    {"flowerbed", LandUseClass::flowerbed},
    {"forest", LandUseClass::forest},
    {"garages", LandUseClass::garages},
    {"garden", LandUseClass::garden},
    {"golf_course", LandUseClass::golf_course},
    {"grass", LandUseClass::grass},
    {"grave_yard", LandUseClass::grave_yard},
    {"green", LandUseClass::green},
    {"greenfield", LandUseClass::greenfield},
    {"greenhouse_horticulture", LandUseClass::greenhouse_horticulture},
    {"highway", LandUseClass::highway},
    {"hike", LandUseClass::hike},
    {"hospital", LandUseClass::hospital},
    {"ice_skate", LandUseClass::ice_skate},
    {"industrial", LandUseClass::industrial},
    {"institutional", LandUseClass::institutional},
    {"kindergarten", LandUseClass::kindergarten},
    {"landfill", LandUseClass::landfill},
    {"lateral_water_hazard", LandUseClass::lateral_water_hazard},
    {"logging", LandUseClass::logging},
    {"marina", LandUseClass::marina},
    {"meadow", LandUseClass::meadow},
    {"military", LandUseClass::military},
    {"military_hospital", LandUseClass::military_hospital},
    {"military_school", LandUseClass::military_school},
    {"music_school", LandUseClass::music_school},
    {"national_park", LandUseClass::national_park},
    {"natural_monument", LandUseClass::natural_monument},
    {"nature_reserve", LandUseClass::nature_reserve},
    {"naval_base", LandUseClass::naval_base},
    {"nordic", LandUseClass::nordic},
    {"nuclear_explosion_site", LandUseClass::nuclear_explosion_site},
    {"obstacle_course", LandUseClass::obstacle_course},
    {"orchard", LandUseClass::orchard},
    {"park", LandUseClass::park},
    {"peat_cutting", LandUseClass::peat_cutting},
    {"pedestrian", LandUseClass::pedestrian},
    {"pitch", LandUseClass::pitch},
    {"plant_nursery", LandUseClass::plant_nursery},
    {"playground", LandUseClass::playground},
    {"plaza", LandUseClass::plaza},
    {"protected", LandUseClass::protected_area},
    {"protected_landscape_seascape",
     LandUseClass::protected_landscape_seascape},
    {"quarry", LandUseClass::quarry},
    {"railway", LandUseClass::railway},
    {"range", LandUseClass::range},
    {"recreation_ground", LandUseClass::recreation_ground},
    {"religious", LandUseClass::religious},
    {"residential", LandUseClass::residential},
    {"resort", LandUseClass::resort},
    {"retail", LandUseClass::retail},
    {"rough", LandUseClass::rough},
    {"salt_pond", LandUseClass::salt_pond},
    {"school", LandUseClass::school},
    {"schoolyard", LandUseClass::schoolyard},
    {"ski_jump", LandUseClass::ski_jump},
    {"skitour", LandUseClass::skitour},
    {"sled", LandUseClass::sled},
    {"sleigh", LandUseClass::sleigh},
    {"snow_park", LandUseClass::snow_park},
    {"species_management_area", LandUseClass::species_management_area},
    {"stadium", LandUseClass::stadium},
    {"state_park", LandUseClass::state_park},
    {"static_caravan", LandUseClass::static_caravan},
    {"strict_nature_reserve", LandUseClass::strict_nature_reserve},
    {"tee", LandUseClass::tee},
    {"theme_park", LandUseClass::theme_park},
    {"track", LandUseClass::track},
    {"traffic_island", LandUseClass::traffic_island},
    {"training_area", LandUseClass::training_area},
    {"trench", LandUseClass::trench},
    {"university", LandUseClass::university},
    {"village_green", LandUseClass::village_green},
    {"vineyard", LandUseClass::vineyard},
    {"water_hazard", LandUseClass::water_hazard},
    {"water_park", LandUseClass::water_park},
    {"wilderness_area", LandUseClass::wilderness_area},
    {"winter_sports", LandUseClass::winter_sports},
    {"works", LandUseClass::works},
    {"zoo", LandUseClass::zoo},
};

auto overture_land_use_class_from_string(const String& name) -> LandUseClass {
    // clang-format off

    // clang-format on
    const auto it = kLandUseMap.find(name);
    return (it != kLandUseMap.end()) ? it->second : LandUseClass::unknown;
}

auto overture_land_class_to_string(LandUseClass luClass) -> String {
    for (const auto& it : kLandUseMap) {
        if (it.second == luClass) {
            return it.first;
        }
    }
    THROW_MSG("Unsupport land use class {}", (I32)luClass);
    return {};
}

// ---------------------------------------------------------------------------
// Overture base/land — class name ↔ LandClass enum
// ---------------------------------------------------------------------------

static const std::unordered_map<std::string, LandClass> kLandClassMap = {
    {"archipelago", LandClass::archipelago},
    {"bare_rock", LandClass::bare_rock},
    {"beach", LandClass::beach},
    {"cave_entrance", LandClass::cave_entrance},
    {"cliff", LandClass::cliff},
    {"desert", LandClass::desert},
    {"dune", LandClass::dune},
    {"fell", LandClass::fell},
    {"forest", LandClass::forest},
    {"glacier", LandClass::glacier},
    {"grass", LandClass::grass},
    {"grassland", LandClass::grassland},
    {"heath", LandClass::heath},
    {"hill", LandClass::hill},
    {"island", LandClass::island},
    {"islet", LandClass::islet},
    {"land", LandClass::land},
    {"meadow", LandClass::meadow},
    {"meteor_crater", LandClass::meteor_crater},
    {"mountain_range", LandClass::mountain_range},
    {"peak", LandClass::peak},
    {"peninsula", LandClass::peninsula},
    {"plateau", LandClass::plateau},
    {"reef", LandClass::reef},
    {"ridge", LandClass::ridge},
    {"rock", LandClass::rock},
    {"saddle", LandClass::saddle},
    {"sand", LandClass::sand},
    {"scree", LandClass::scree},
    {"scrub", LandClass::scrub},
    {"shingle", LandClass::shingle},
    {"shrub", LandClass::shrub},
    {"shrubbery", LandClass::shrubbery},
    {"stone", LandClass::stone},
    {"tree", LandClass::tree},
    {"tree_row", LandClass::tree_row},
    {"tundra", LandClass::tundra},
    {"valley", LandClass::valley},
    {"volcanic_caldera_rim", LandClass::volcanic_caldera_rim},
    {"volcano", LandClass::volcano},
    {"wetland", LandClass::wetland},
    {"wood", LandClass::wood},
};

auto overture_land_class_from_string(const String& name) -> LandClass {
    const auto it = kLandClassMap.find(name);
    return (it != kLandClassMap.end()) ? it->second : LandClass::unknown;
}

auto overture_land_class_to_string(LandClass cls) -> String {
    for (const auto& it : kLandClassMap) {
        if (it.second == cls)
            return it.first;
    }
    THROW_MSG("Unsupported land class {}", (I32)cls);
    return {};
}

} // namespace nv