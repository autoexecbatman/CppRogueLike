// file: Ai.h
#ifndef AI_H
#define AI_H

#pragma once

#include <curses.h>

#include "../Persistent/Persistent.h"

class Creature; // for no circular dependency with Creature.h
struct GameContext; // for dependency injection

//==AI==
class Ai : public Persistent
{
public:
	virtual ~Ai() {};

	virtual void update(Creature& owner, GameContext& ctx) = 0;

	// Type-safe hostility check - replaces dynamic_cast usage
	virtual bool is_hostile() const { return true; } // Most AI types are hostile by default

	static std::unique_ptr<Ai> create(const json& j);

	int calculate_step(int positionDifference); // utility
	int get_next_level_xp(GameContext& ctx, Creature& owner);
	void levelup_update(GameContext& ctx, Creature& owner);
	int calculate_fighter_xp(int level);
	int calculate_rogue_xp(int level);
	int calculate_cleric_xp(int level);
	int calculate_wizard_xp(int level);
protected:
	enum class AiType
	{
		MONSTER, CONFUSED_MONSTER, PLAYER, SHOPKEEPER
	};


};

//====

#endif // !AI_H
