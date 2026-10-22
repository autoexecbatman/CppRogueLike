// LevelManager.cpp - Handles dungeon level progression and level-specific state
#include <cassert>
#include <format>

#include <nlohmann/json.hpp>

#include "Colors.h"
#include "GameContext.h"
#include "LevelManager.h"
#include "Map.h"
#include "MessageSystem.h"
#include "Player.h"

void LevelManager::advance_to_next_level(GameContext& ctx)
{
	assert(ctx.map && "LevelManager::advance_to_next_level: called without a map");

	dungeon_level++;
	shopkeepers_on_current_level = 0; // Reset shopkeeper counter for new level

	// Display progression messages
	display_level_messages(*ctx.messageSystem);

	// A new level is a new square, even one at the same coordinates as the hiding spot.
	if (ctx.player_concrete().is_in_shadows())
	{
		ctx.player_concrete().leave_shadows(ctx);
	}

	// Heal player between levels
	heal_player_between_levels(ctx);

	// Regenerate the map for new level
	ctx.map->regenerate(ctx);
}

void LevelManager::reset_to_first_level()
{
	dungeon_level = 1;
	shopkeepers_on_current_level = 0;
}

bool LevelManager::can_spawn_shopkeeper(int max_shopkeepers) const noexcept
{
	return shopkeepers_on_current_level < max_shopkeepers;
}

void LevelManager::save_to_json(nlohmann::json& output) const
{
	output["level_manager"] = {
		{ "dungeonLevel", dungeon_level },
		{ "shopkeepersOnCurrentLevel", shopkeepers_on_current_level },
	};
}

void LevelManager::load_from_json(const nlohmann::json& source)
{
	const auto& levelData = source.at("level_manager");
	dungeon_level = levelData.at("dungeonLevel").get<int>();
	shopkeepers_on_current_level = levelData.at("shopkeepersOnCurrentLevel").get<int>();
}

void LevelManager::display_level_messages(MessageSystem& message_system) const
{
	message_system.message(ColorPairId::WHITE_BLACK, "You take a moment to rest, and recover your strength.", MessageCompletion::FINISHED);
	message_system.message(ColorPairId::WHITE_BLACK, "After a rare moment of peace, you descend", MessageCompletion::FINISHED);
	message_system.message(ColorPairId::WHITE_BLACK, "deeper into the heart of the dungeon...", MessageCompletion::FINISHED);
	message_system.message(ColorPairId::WHITE_BLACK, std::format("You are now on level {}", dungeon_level), MessageCompletion::FINISHED);
}

void LevelManager::heal_player_between_levels(GameContext& ctx) const
{
	if (!ctx.player())
	{
		return;
	}

	const int healAmount = ctx.player()->get_max_hp() / 2;
	const int actualHealed = ctx.player()->heal(healAmount);

	if (actualHealed > 0)
	{
		ctx.messageSystem->message(ColorPairId::GREEN_BLACK,
			std::format("You rest between levels and recover {} HP.", actualHealed),
			MessageCompletion::FINISHED);
	}
}
