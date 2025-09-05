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
	// Retrieve the player's current level
	int currentLevel = game.player->playerLevel;

	// Use AD&D 2e XP tables based on class
	switch (game.player->playerClassState)
	{
	case Player::PlayerClassState::FIGHTER:
		// Fighters use a moderate progression
		return calculate_fighter_xp(currentLevel);

	case Player::PlayerClassState::ROGUE:
		// Rogues level up more quickly in early levels
		return calculate_rogue_xp(currentLevel);

	case Player::PlayerClassState::CLERIC:
		// Clerics have a steady progression
		return calculate_cleric_xp(currentLevel);

	case Player::PlayerClassState::WIZARD:
		// Wizards require the most XP
		return calculate_wizard_xp(currentLevel);

	default:
		// Default fallback (simplified progression)
		return 2000 * currentLevel;
	}
}

void Ai::levelup_update(Creature& owner)
{
	game.log("AiPlayer::levelUpUpdate(Actor& owner)");
	// level up if needed
	int levelUpXp = get_next_level_xp(owner);

	if (owner.destructible->get_xp() >= levelUpXp)
	{
		game.player->playerLevel++;
		owner.destructible->set_xp(owner.destructible->get_xp() - levelUpXp);
		game.message(WHITE_BLACK_PAIR, std::format("Your battle skills grow stronger! You reached level {}", game.player->playerLevel), true);
		
		game.display_levelup(game.player->playerLevel);
	}
}

int Ai::calculate_fighter_xp(int level)
{
    // AD&D 2e Fighter XP progression
    static const int fighterXP[] = {
        0,
		2000, // need 2000 XP to reach level 2
        4000,  
        8000,  
        16000,
        32000,
        64000,
        125000,
        250000,
        500000,
        750000
    };

    // Cap at array size, then use linear progression
    if (level < static_cast<int>(sizeof(fighterXP) / sizeof(fighterXP[0]))) {
        return fighterXP[level];
    }
    else {
        return fighterXP[10] + (level - 10) * 250000;
    }
}

int Ai::calculate_rogue_xp(int level)
{
    // AD&D 2e Thief/Rogue XP progression
    static const int rogueXP[] = {
        0,
		1250, // need 1250 XP to reach level 2
        2500,
        5000,
        10000,
        20000,
        40000,
        70000,
        110000,
        160000,
        220000
    };

    // Cap at array size, then use linear progression
    if (level < static_cast<int>(sizeof(rogueXP) / sizeof(rogueXP[0]))) {
        return rogueXP[level];
    }
    else {
        return rogueXP[10] + (level - 10) * 60000;
    }
}

int Ai::calculate_cleric_xp(int level)
{
    // AD&D 2e Cleric XP progression
	static const int clericXP[] = {
		0,
		1500, // need 1500 XP to reach level 2
		3000,
		6000,
		13000,
		27500,
		55000,
		110000,
		225000,
		450000,
		675000
	};

    // Cap at array size, then use linear progression
    if (level < static_cast<int>(sizeof(clericXP) / sizeof(clericXP[0]))) {
        return clericXP[level];
    }
    else {
        return clericXP[10] + (level - 10) * 225000;
    }
}

int Ai::calculate_wizard_xp(int level)
{
    // AD&D 2e Wizard/Mage XP progression
	static const int wizardXP[] = {
		0,
		2500,
		5000,
		10000,
		20000,
		40000,
		60000,
		90000,
		135000,
		250000,
		375000
	};

    // Cap at array size, then use linear progression
    if (level < static_cast<int>(sizeof(wizardXP) / sizeof(wizardXP[0]))) {
        return wizardXP[level];
    }
    else {
        return wizardXP[10] + (level - 10) * 125000;
    }
}

// end of file: Ai.cpp
