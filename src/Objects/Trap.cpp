// file: Trap.cpp
// Implementation of trap mechanics: detection, triggering, disarming, damage

#include "Trap.h"
#include "../Core/GameContext.h"
#include "../Random/RandomDice.h"
#include "../Actor/Creature.h"
#include "../Map/Map.h"
#include "../Systems/TileConfig.h"
#include "../Systems/MessageSystem.h"
#include <algorithm>
#include <ranges>

Trap::Trap(Vector2D position, TrapType type, const TileConfig& tileConfig)
	: TileFeature(position, ActorData{}, FeatureKind::TRAP),
	type_(type),
	state_(TrapState::HIDDEN),
	detectionDC_(15),
	disarmDC_(12)
{
	// Set damage dice and display name based on trap type
	std::string trapName;
	TileRef trapTile;
	switch (type)
	{
	case TrapType::PIT:
		damageDiceCount_ = 2;
		damageDiceSize_ = 6;  // 2d6 damage
		trapName = "pit trap";
		trapTile = tileConfig.get("TILE_FLOOR_STONE");  // Hidden on floor
		break;
	case TrapType::DART:
		damageDiceCount_ = 1;
		damageDiceSize_ = 4;  // 1d4 damage
		trapName = "dart trap";
		trapTile = tileConfig.get("TILE_FLOOR_STONE");  // Hidden on floor
		break;
	case TrapType::ARROW:
		damageDiceCount_ = 1;
		damageDiceSize_ = 6;  // 1d6 damage
		trapName = "arrow trap";
		trapTile = tileConfig.get("TILE_FLOOR_STONE");  // Hidden on floor
		break;
	}

	actorData.name = trapName;
	actorData.tile = trapTile;
	actorData.color = YELLOW_BLACK_PAIR;

	// Hidden traps are invisible until detected
	add_state(ActorState::IS_INVISIBLE);
}

bool Trap::attempt_detect(Creature& creature, GameContext& ctx)
{
	if (state_ != TrapState::HIDDEN)
	{
		return false;  // Already detected, disarmed, or triggered
	}

	// Roll 1d20 + DEX modifier vs detection DC
	int roll = ctx.dice->roll(1, 20);
	int dexMod = (creature.get_dexterity() - 10) / 2;
	int checkResult = roll + dexMod;

	if (checkResult >= detectionDC_)
	{
		state_ = TrapState::DETECTED;
		remove_state(ActorState::IS_INVISIBLE);  // Trap is now visible
		if (ctx.messageSystem)
		{
			ctx.messageSystem->message(YELLOW_BLACK_PAIR, "You notice a hidden trap!", true);
		}
		return true;
	}

	return false;
}

DisarmResult Trap::attempt_disarm(Creature& creature, GameContext& ctx)
{
	// Nothing left to disarm.
	if (state_ == TrapState::DISARMED)
	{
		return DisarmResult::ALREADY_DISARMED;
	}

	// A trap the creature has not spotted cannot be worked on.
	if (state_ == TrapState::HIDDEN)
	{
		return DisarmResult::NOT_VISIBLE;
	}

	// Roll 1d20 + DEX modifier vs disarm DC
	int roll = ctx.dice->roll(1, 20);
	int dexMod = (creature.get_dexterity() - 10) / 2;
	int checkResult = roll + dexMod;

	if (checkResult >= disarmDC_)
	{
		state_ = TrapState::DISARMED;
		return DisarmResult::DISARMED;
	}

	// A failed attempt sets the trap off on the creature working on it.
	state_ = TrapState::TRIGGERED;
	on_creature_enter(creature, ctx);

	return DisarmResult::TRIGGERED;
}

EntryResult Trap::on_creature_enter(Creature& creature, GameContext& ctx)
{
	// Passive detection check when walking into trap
	attempt_passive_detection(creature, ctx);

	// If already detected or triggered, apply damage
	if (state_ == TrapState::DETECTED || state_ == TrapState::TRIGGERED)
	{
		// Trigger the trap
		state_ = TrapState::TRIGGERED;

		// Roll damage
		int damage = roll_damage(*ctx.dice);

		// Apply damage to creature
		if (ctx.messageSystem)
		{
			std::string trapName;
			switch (type_)
			{
			case TrapType::PIT:
				trapName = "pit";
				break;
			case TrapType::DART:
				trapName = "dart trap";
				break;
			case TrapType::ARROW:
				trapName = "arrow trap";
				break;
			}
			ctx.messageSystem->message(RED_BLACK_PAIR,
				"You trigger the " + trapName + " and take " + std::to_string(damage) + " damage!", true);
		}

		creature.take_damage_and_check_death(damage, ctx);

		// 50% chance trap is destroyed after triggering
		if (ctx.dice->d2() == 1)
		{
			destroy();
			if (ctx.messageSystem)
			{
				ctx.messageSystem->message(WHITE_BLACK_PAIR, "The trap is destroyed.", true);
			}
		}

		// Trap blocks movement on first trigger (only if creature still alive)
		if (creature.get_hp() > 0)
		{
			ctx.gameState->set_game_status(GameStatus::NEW_TURN);
		}
		return EntryResult::BLOCKED;
	}

	// Hidden trap didn't trigger (missed detection)
	return EntryResult::UNAFFECTED;
}

void Trap::attempt_passive_detection(Creature& creature, GameContext& ctx)
{
	if (state_ == TrapState::HIDDEN)
	{
		// Passive detection: roll vs detection DC
		int roll = ctx.dice->roll(1, 20);
		int dexMod = (creature.get_dexterity() - 10) / 2;
		int checkResult = roll + dexMod;

		if (checkResult >= detectionDC_)
		{
			state_ = TrapState::DETECTED;
			if (ctx.messageSystem)
			{
				ctx.messageSystem->message(YELLOW_BLACK_PAIR, "You notice a hidden trap at the last moment!", true);
			}
		}
	}
}

int Trap::roll_damage(RandomDice& dice) const
{
	return dice.roll(damageDiceCount_, damageDiceSize_);
}



void Trap::destroy()
{
	mark_destroyed();
}
