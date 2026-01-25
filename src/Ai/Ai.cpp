// file: Ai.cpp
#include <iostream>
#include <format>
#include <array>

#include "AiMonster.h"
#include "AiMonsterConfused.h"
#include "AiPlayer.h"
#include "AiShopkeeper.h"
#include "../Core/GameContext.h"
#include "../ActorTypes/Player.h"
#include "../Systems/MessageSystem.h"
#include "../Systems/DisplayManager.h"

//==AI==
std::unique_ptr<Ai> Ai::create(const json& j)
{
	if (!j.contains("type") || !j["type"].is_number())
	{
		throw std::runtime_error("Invalid JSON format: Missing or invalid 'type'");
	}

	auto type = static_cast<AiType>(j["type"].get<int>());
	std::unique_ptr<Ai> ai;

	switch (type)
	{
	case AiType::PLAYER:
		ai = std::make_unique<AiPlayer>();
		break;
	case AiType::MONSTER:
		ai = std::make_unique<AiMonster>();
		break;
	case AiType::CONFUSED_MONSTER:
		ai = std::make_unique<AiMonsterConfused>(0, nullptr);
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

int Ai::get_next_level_xp(GameContext& ctx, Creature& owner)
{
	// Retrieve the player's current level
	int currentLevel = ctx.player->get_player_level();

	// Use AD&D 2e XP tables based on class
	switch (ctx.player->playerClassState)
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

void Ai::levelup_update(GameContext& ctx, Creature& owner)
{
	ctx.message_system->log("AiPlayer::levelUpUpdate(Actor& owner)");
	// level up if needed
	int levelUpXp = get_next_level_xp(ctx, owner);
	if (owner.destructible->get_xp() >= levelUpXp)
	{
		ctx.player->adjust_level(1);
		owner.destructible->set_xp(owner.destructible->get_xp() - levelUpXp);
		ctx.message_system->message(WHITE_BLACK_PAIR, std::format("Your battle skills grow stronger! You reached level {}", ctx.player->get_player_level()), true);
		ctx.display_manager->display_levelup(*ctx.player, ctx.player->get_player_level(), ctx);
	}
}

[[nodiscard]] constexpr int Ai::calculate_fighter_xp(int level) noexcept
{
    // AD&D 2e Fighter XP progression
    constexpr std::array fighter_xp = {
        0, 2000, 4000, 8000, 16000, 32000, 
        64000, 125000, 250000, 500000, 750000
    };
    return calculate_xp_for_level(level, fighter_xp, 250000);
}

[[nodiscard]] constexpr int Ai::calculate_rogue_xp(int level) noexcept
{
    // AD&D 2e Thief/Rogue XP progression
    constexpr std::array rogue_xp = {
        0, 1250, 2500, 5000, 10000, 20000,
        40000, 70000, 110000, 160000, 220000
    };
    return calculate_xp_for_level(level, rogue_xp, 60000);
}

[[nodiscard]] constexpr int Ai::calculate_cleric_xp(int level) noexcept
{
    // AD&D 2e Cleric XP progression
    constexpr std::array cleric_xp = {
        0, 1500, 3000, 6000, 13000, 27500,
        55000, 110000, 225000, 450000, 675000
    };
    return calculate_xp_for_level(level, cleric_xp, 225000);
}

[[nodiscard]] constexpr int Ai::calculate_wizard_xp(int level) noexcept
{
    // AD&D 2e Wizard/Mage XP progression
    constexpr std::array wizard_xp = {
        0, 2500, 5000, 10000, 20000, 40000,
        60000, 90000, 135000, 250000, 375000
    };
    return calculate_xp_for_level(level, wizard_xp, 125000);
}

// end of file: Ai.cpp
