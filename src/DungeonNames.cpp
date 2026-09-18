// file: DungeonNames.cpp
#include <array>
#include <format>
#include <string_view>

#include "DungeonNames.h"

namespace DungeonNames
{
std::string generate_warden_name(RandomDice& rng)
{
    using namespace std::string_view_literals;

    static constexpr std::array firstNames{
        "Valdus"sv,   "Grimholt"sv, "Korrigan"sv, "Branag"sv,  "Theron"sv,
        "Malgrath"sv, "Durkon"sv,   "Vorith"sv,   "Hendrek"sv, "Bragus"sv,
        "Orlan"sv,    "Skarok"sv,   "Vethis"sv,   "Drakan"sv,  "Lothar"sv
    };
    static constexpr std::array epithets{
        "the Cruel"sv,      "the Iron"sv,       "the Merciless"sv, "the Grim"sv,
        "the Bloody"sv,     "the Relentless"sv, "the Dread"sv,     "the Unyielding"sv,
        "Ironhand"sv,       "Stoneskin"sv,      "the Pitiless"sv,  "the Warden"sv,
        "Blackchain"sv,     "the Unbroken"sv,   "the Ruthless"sv
    };

    const int nameIdx = rng.roll(0, static_cast<int>(firstNames.size()) - 1);
    const int epithetIdx = rng.roll(0, static_cast<int>(epithets.size()) - 1);
    return std::format("{} {}", firstNames[nameIdx], epithets[epithetIdx]);
}
}
