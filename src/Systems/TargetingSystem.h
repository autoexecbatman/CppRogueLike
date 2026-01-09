#pragma once

#include "../Utils/Vector2D.h"
#include "../Actor/Actor.h"

struct GameContext;

class TargetingSystem
{
public:
	const Vector2D select_target(const GameContext& ctx, Vector2D startPos, int maxRange) const;
	bool pick_tile(GameContext& ctx, Vector2D* position, int maxRange) const;  // Game.cpp compatibility method
	void draw_los(const GameContext& ctx, Vector2D targetCursor) const;
	void draw_range_indicator(GameContext& ctx, Vector2D center, int range) const;
	void draw_aoe_preview(Vector2D center, int radius) const;  // AOE preview from pick_tile - no game access
	bool is_valid_target(const GameContext& ctx, Vector2D from, Vector2D to, int maxRange) const;
	bool process_ranged_attack(GameContext& ctx, Creature& attacker, Vector2D targetPos) const;
	void animate_projectile(GameContext& ctx, Vector2D from, Vector2D to, char projectileSymbol, int colorPair) const;

	// Handle ranged attack coordination
	void handle_ranged_attack(GameContext& ctx) const;
};