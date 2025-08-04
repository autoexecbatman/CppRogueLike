#pragma once

#include "../Utils/Vector2D.h"
#include "../Actor/Actor.h"

class TargetingSystem
{

public:
	Vector2D select_target(Vector2D startPos, int maxRange);
	void draw_los(Vector2D targetCursor);
	void draw_range_indicator(Vector2D center, int range);
	bool is_valid_target(Vector2D from, Vector2D to, int maxRange);
	bool process_ranged_attack(Creature& attacker, Vector2D targetPos);
	void animate_projectile(Vector2D from, Vector2D to, char projectileSymbol, int colorPair);
};