#pragma once

#include <string_view>

class Creature;

namespace Vision
{

// The registry key of the only light source this game carries.
inline constexpr std::string_view TORCH_ITEM_KEY = "torch";

// How far a torchbearer sees, in tiles. Table 63 of the Player's Handbook gives a
// torch a 15-foot radius, which this game cannot convert because it states no
// feet-per-tile scale anywhere. Six is taken from the light mask instead: the gradient
// in RenderingManager is painted out to 6.5 tiles, so sight and light agree here and
// the authored falloff is almost fully used.
inline constexpr int TORCH_SIGHT_RADIUS = 6;

// How far this creature can see, in tiles: the base radius, widened while a torch is
// in its pack. Recomputed from the inventory on every call, so no stored radius can
// disagree with what is actually carried.
//
// Burning time is not modelled. Table 63 gives a torch thirty minutes; one here never
// goes out.
//
// Example:
//   sight_radius(emptyHandedRogue);     // -> 4, the base radius
//   sight_radius(rogueCarryingAStone);  // -> 4, only a torch counts
//   sight_radius(rogueCarryingATorch);  // -> 6
[[nodiscard]] int sight_radius(const Creature& viewer) noexcept;

} // namespace Vision
