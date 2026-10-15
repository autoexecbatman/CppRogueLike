// file: CostOverlay.h
// A debug view of the cost map monsters step by. Every tile the flood from the player
// reached shows its cost, on a tint running from warm beside the player to cool at the
// farthest reached tile; a tile the flood never reached is left bare. The overlay starts
// hidden, and `v` toggles it in a Debug build.
//
// Example:
//   CostOverlay overlay{};
//   overlay.is_visible();              // -> false
//   overlay.toggle();
//   overlay.is_visible();              // -> true, drawn from the next frame
//
//   dijkstra_cost_label(7);            // -> "7"
//   dijkstra_cost_label(INT_MAX);      // -> nullopt, the flood never reached the tile
//   dijkstra_cost_tint(0, 40);         // -> COST_NEAR_TINT, the player's own tile
//   dijkstra_cost_tint(40, 40);        // -> COST_FAR_TINT, the farthest reached tile
#pragma once

#include <optional>
#include <string>

#include <raylib.h>

struct GameContext;

inline constexpr Color COST_NEAR_TINT{ 230, 70, 40, 120 };
inline constexpr Color COST_FAR_TINT{ 40, 110, 230, 120 };

// The text a tile shows for its cost, or nothing for a tile the flood never reached.
std::optional<std::string> dijkstra_cost_label(int cost);

// The tint for a reached tile, a straight blend from COST_NEAR_TINT at cost 0 to
// COST_FAR_TINT at farthestCost. Requires 0 <= cost <= farthestCost; when the player's
// own tile is the only one reached, farthestCost is 0 and the tint is COST_NEAR_TINT.
Color dijkstra_cost_tint(int cost, int farthestCost);

class CostOverlay
{
private:
	bool visible{ false };

public:
	void toggle() noexcept;
	[[nodiscard]] bool is_visible() const noexcept;

	// Draws every on-screen reached tile's cost over the lit map. Does nothing while
	// hidden.
	void render(const GameContext& ctx) const;
};
