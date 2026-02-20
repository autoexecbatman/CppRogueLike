#pragma once

#include <vector>
#include "../Utils/Vector2D.h"
#include "../Actor/Actor.h"
#include "TargetMode.h"

struct GameContext;

struct TargetResult {
    bool success{false};
    Vector2D impact_pos{0, 0};
    std::vector<Creature*> creatures;
};

class TargetingSystem
{
public:
	const Vector2D select_target(GameContext& ctx, Vector2D startPos, int maxRange) const;
	bool pick_tile(GameContext& ctx, Vector2D* position, int maxRange) const;
	bool pick_tile_aoe(GameContext& ctx, Vector2D* position, int maxRange, int aoe_radius) const;
	void draw_los(GameContext& ctx, Vector2D targetCursor) const;
	void draw_range_indicator(GameContext& ctx, Vector2D center, int range) const;
	void draw_aoe_preview(GameContext& ctx, Vector2D center, int radius) const;
	bool is_valid_target(GameContext& ctx, Vector2D from, Vector2D to, int maxRange) const;
	bool process_ranged_attack(GameContext& ctx, Creature& attacker, Vector2D targetPos) const;
	void animate_projectile(GameContext& ctx, Vector2D from, Vector2D to, char projectileSymbol, int colorPair) const;

	// Handle ranged attack coordination
	void handle_ranged_attack(GameContext& ctx) const;

	TargetResult acquire_targets(GameContext& ctx, TargetMode mode, Vector2D origin, int range, int aoe_radius = 0) const;

private:
	// Get weapon range based on weapon type (AD&D 2e ranges in dungeon tiles)
	static int get_weapon_range(const Item* weapon);

	TargetResult target_auto_nearest(GameContext& ctx, Vector2D origin, int range) const;
	TargetResult target_pick_single(GameContext& ctx) const;
	TargetResult target_pick_aoe(GameContext& ctx, int aoe_radius) const;

	bool run_targeting_loop(GameContext& ctx, Vector2D* position, int maxRange, int aoe_radius) const;
};
