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

	if (owner.destructible->xp >= levelUpXp)
	{
		game.player->playerLevel++;
		owner.destructible->xp -= levelUpXp;
		game.message(WHITE_PAIR, std::format("Your battle skills grow stronger! You reached level {}", game.player->playerLevel), true);
		
		game.dispay_levelup(game.player->playerLevel);
	}
}

int Ai::calculate_fighter_xp(int level)
{
    // AD&D 2e Fighter XP progression
    static const int fighterXP[] = {
        0,      // Level 0 (not used)
        0,      // Level 1
        2000,   // Level 2
        4000,   // Level 3
        8000,   // Level 4
        16000,  // Level 5
        32000,  // Level 6
        64000,  // Level 7
        125000, // Level 8
        250000, // Level 9
        500000, // Level 10
        750000  // Level 11+
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
        0,      // Level 0 (not used)
        0,      // Level 1
        1250,   // Level 2
        2500,   // Level 3
        5000,   // Level 4
        10000,  // Level 5
        20000,  // Level 6
        40000,  // Level 7
        70000,  // Level 8
        110000, // Level 9
        160000, // Level 10
        220000  // Level 11+
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
        0,      // Level 0 (not used)
        0,      // Level 1
        1500,   // Level 2
        3000,   // Level 3
        6000,   // Level 4
        13000,  // Level 5
        27500,  // Level 6
        55000,  // Level 7
        110000, // Level 8
        225000, // Level 9
        450000, // Level 10
        675000  // Level 11+
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
        0,      // Level 0 (not used)
        0,      // Level 1
        2500,   // Level 2
        5000,   // Level 3
        10000,  // Level 4
        20000,  // Level 5
        40000,  // Level 6
        60000,  // Level 7
        90000,  // Level 8
        135000, // Level 9
        250000, // Level 10
        375000  // Level 11+
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
