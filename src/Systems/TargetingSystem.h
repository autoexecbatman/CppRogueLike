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
	TargetResult acquire_targets(GameContext& ctx, TargetMode mode, Vector2D origin, int range, int aoe_radius) const;

private:
	static int get_weapon_range(const Item* weapon);
	TargetResult target_auto_nearest(GameContext& ctx, Vector2D origin, int range) const;
};
