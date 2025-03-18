// file: Ai.cpp
#include <iostream>
#include <libtcod.h>
#include <format>

#include "AiMonster.h"
#include "AiMonsterConfused.h"
#include "AiPlayer.h"
#include "AiShopkeeper.h"
#include "../Game.h"

//==AI==
std::unique_ptr<Ai> Ai::create(const json& j)
{
	if (!j.contains("type") || !j["type"].is_number()) {
		throw std::runtime_error("Invalid JSON format: Missing or invalid 'type'");
	}

	auto type = static_cast<AiType>(j["type"].get<int>());
	std::unique_ptr<Ai> ai;

	switch (type) {
	case AiType::PLAYER:
		ai = std::make_unique<AiPlayer>();
		break;
	case AiType::MONSTER:
		ai = std::make_unique<AiMonster>();
		break;
	case AiType::CONFUSED_MONSTER:
		ai = std::make_unique<AiMonsterConfused>(0,nullptr);
		break;
	case AiType::SHOPKEEPER:
		ai = std::make_unique<AiShopkeeper>();
		break;
	default:
		throw std::runtime_error("Unknown AiType");
	} // end of switch (type)

	ai->load(j);
	return ai;
}

// If positionDifference > 0, return 1; otherwise, return -1
int Ai::calculate_step(int positionDifference)
{
	return positionDifference > 0 ? 1 : -1;
}

int Ai::get_next_level_xp(Creature& owner)
{
	constexpr int LEVEL_UP_BASE = 200;
	constexpr int LEVEL_UP_FACTOR = 150;

	return LEVEL_UP_BASE + (owner.playerLevel * LEVEL_UP_FACTOR);
}


void Ai::levelup_update(Creature& owner)
{
	game.log("AiPlayer::levelUpUpdate(Actor& owner)");
	// level up if needed
	int levelUpXp = get_next_level_xp(owner);

	if (owner.destructible->xp >= levelUpXp)
	{
		game.player->playerLevel++;
		owner.destructible->xp -= levelUpXp;
		game.message(WHITE_PAIR, std::format("Your battle skills grow stronger! You reached level {}", game.player->playerLevel), true);
		game.player->calculate_thaco();
		game.dispay_levelup(game.player->playerLevel);
	}
}
//====

// end of file: Ai.cpp
