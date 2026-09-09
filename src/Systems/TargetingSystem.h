#pragma once

#include <vector>

#include "../Actor/Actor.h"
#include "../Utils/Vector2D.h"
#include "TargetMode.h"

struct GameContext;

struct TargetResult
{
	bool success{ false };
	std::vector<Creature*> creatures;
};

class TargetingSystem
{
public:
	void draw_los(GameContext& ctx, Vector2D targetCursor) const;
	void draw_range_indicator(GameContext& ctx, Vector2D center, int range) const;
	void draw_aoe_preview(GameContext& ctx, Vector2D center, int radius) const;
	bool is_valid_target(GameContext& ctx, Vector2D from, Vector2D to, int maxRange) const;
	void handle_ranged_attack(GameContext& ctx) const;
	// Picks the nearest hostile creature within range of origin, immediately.
	//
	// This is the synchronous half of targeting. Modes that need the player to
	// choose a tile - PICK_TILE_SINGLE, PICK_TILE_AOE - cannot be answered in a
	// single call and go through TargetingMenu instead; see Pickable's scroll
	// handling for that path.
	//
	// Example:
	//   const TargetResult result = targeting.acquire_nearest(ctx, player.position, 6);
	//   result.success;          // -> false when nothing hostile is in range
	//   result.creatures.size(); // -> 1 on success
	TargetResult acquire_nearest(GameContext& ctx, Vector2D origin, int range) const;

private:
	static int get_weapon_range(const Item* weapon);
	TargetResult target_auto_nearest(GameContext& ctx, Vector2D origin, int range) const;
};
