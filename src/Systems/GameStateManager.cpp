// GameStateManager.cpp - Handles game state persistence and level management
#include <cassert>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <system_error>
#include <utility>
#include <vector>

#include <nlohmann/json_fwd.hpp>

#include "../Actor/Actor.h"
#include "../Actor/InventoryData.h"
#include "../Actor/InventoryOperations.h"
#include "../ActorTypes/Player.h"
#include "../Colors/Colors.h"
#include "../Core/GameContext.h"
#include "../Core/Paths.h"
#include "../Gui/Gui.h"
#include "../Map/DungeonRoom.h"
#include "../Map/Map.h"
#include "../Systems/DataManager.h"
#include "../Systems/HungerSystem.h"
#include "../Systems/LevelManager.h"
#include "../Systems/MenuManager.h"
#include "../Systems/MessageSystem.h"
#include "../Utils/Vector2D.h"
#include "ContentRegistry.h"
#include "GameStateManager.h"

using json = nlohmann::json;
using namespace InventoryOperations;

void GameStateManager::init_new_game(GameContext& ctx)
{
	assert(ctx.data_manager != nullptr);
	assert(ctx.message_system != nullptr);
	assert(ctx.level_manager != nullptr);
	assert(ctx.time != nullptr);
	assert(ctx.isLoadedGame != nullptr);
	assert(ctx.map != nullptr);
	assert(ctx.player != nullptr);
	assert(ctx.game_status != nullptr);

	ContentRegistry::instance().load(Paths::CONTENT_TILES);
	ctx.data_manager->load_all_data(*ctx.message_system);

	ctx.level_manager->reset_to_first_level();
	*ctx.time = 0;
	*ctx.isLoadedGame = false;

	ctx.map->regenerate(ctx);

	ctx.player->roll_new_character(ctx);
	*ctx.game_status = GameStatus::STARTUP;
	ctx.message_system->log("New game initialized");
}

bool GameStateManager::load_all(GameContext& ctx)
{
	assert(ctx.menu_manager != nullptr);
	assert(ctx.data_manager != nullptr);
	assert(ctx.message_system != nullptr);
	assert(ctx.isLoadedGame != nullptr);
	assert(ctx.game_status != nullptr);

	ContentRegistry::instance().load(Paths::CONTENT_TILES);
	ctx.menu_manager->set_game_initialized(true);
	ctx.data_manager->load_all_data(*ctx.message_system);

	if (!load_game(ctx))
	{
		ctx.message_system->log("Error: Could not open save file.");
		return false;
	}

	*ctx.isLoadedGame = true;
	*ctx.game_status = GameStatus::STARTUP;
	ctx.message_system->log("GameStatus set to STARTUP after loading for FOV computation");
	return true;
}

void GameStateManager::save_game(GameContext& ctx)
{
	assert(ctx.map != nullptr);
	assert(ctx.rooms != nullptr);
	assert(ctx.player != nullptr);
	assert(ctx.stairs != nullptr);
	assert(ctx.creatures != nullptr);
	assert(ctx.inventory_data != nullptr);
	assert(ctx.gui != nullptr);
	assert(ctx.hunger_system != nullptr);
	assert(ctx.level_manager != nullptr);
	assert(ctx.time != nullptr);

	auto save_path = Paths::resolve(Paths::SAVE_FILE);
	std::filesystem::create_directories(save_path.parent_path());
	std::ofstream file(save_path);
	if (file.is_open())
	{
		json j;

		ctx.map->save(j);
		save_rooms_to_json(*ctx.rooms, j);

		json playerJson;
		ctx.player->save(playerJson);
		j["player"] = playerJson;

		json stairsJson;
		ctx.stairs->save(stairsJson);
		j["stairs"] = stairsJson;

		save_creatures_to_json(*ctx.creatures, j);
		save_inventory(*ctx.inventory_data, j);

		json guiJson;
		ctx.gui->save(guiJson);
		j["gui"] = guiJson;

		json hungerJson;
		ctx.hunger_system->save(hungerJson);
		j["hunger_system"] = hungerJson;

		ctx.level_manager->save_to_json(j);
		j["time"] = *ctx.time;

		// Write the JSON data to the file
		file << j.dump(4); // Pretty print with an indentation of 4 spaces
		file.close();
	}
	else
	{
		throw std::runtime_error("Error occurred while saving the game.");
	}
}

bool GameStateManager::load_game(GameContext& ctx)
{
	assert(ctx.map != nullptr);
	assert(ctx.rooms != nullptr);
	assert(ctx.player != nullptr);
	assert(ctx.stairs != nullptr);
	assert(ctx.creatures != nullptr);
	assert(ctx.inventory_data != nullptr);
	assert(ctx.gui != nullptr);
	assert(ctx.hunger_system != nullptr);
	assert(ctx.level_manager != nullptr);
	assert(ctx.time != nullptr);

	std::ifstream file(Paths::resolve(Paths::SAVE_FILE));
	if (!file.is_open())
	{
		return false;
	}

	json j;
	file >> j;

	ctx.map->load(j);
	load_rooms_from_json(j, *ctx.rooms);

	if (j.contains("player"))
	{
		ctx.player->load(j["player"]);
	}

	if (j.contains("stairs"))
	{
		ctx.stairs->load(j["stairs"]);
	}

	load_creatures_from_json(j, *ctx.creatures);
	load_inventory(*ctx.inventory_data, j);

	if (j.contains("gui"))
	{
		ctx.gui->load(j["gui"]);
	}

	if (j.contains("hunger_system"))
	{
		ctx.hunger_system->load(ctx, j["hunger_system"]);
	}

	ctx.level_manager->load_from_json(j);

	if (j.contains("time"))
	{
		*ctx.time = j["time"];
	}

	return true; // Successfully loaded
}

bool GameStateManager::save_file_exists()
{
	std::ifstream file(Paths::resolve(Paths::SAVE_FILE));
	return file.good();
}

bool GameStateManager::delete_save_file()
{
	std::error_code ec;
	auto save_path = Paths::resolve(Paths::SAVE_FILE);

	if (!std::filesystem::exists(save_path, ec))
		return true; // File doesn't exist, nothing to delete

	const bool removed = std::filesystem::remove(save_path, ec);
	return removed && !ec;
}

void GameStateManager::save_rooms_to_json(const std::vector<DungeonRoom>& rooms, nlohmann::json& j) const
{
	j["rooms"] = json::array();
	for (const auto& room : rooms)
	{
		j["rooms"].push_back({ { "col", room.col },
			{ "row", room.row },
			{ "width", room.width },
			{ "height", room.height } });
	}
}

void GameStateManager::load_rooms_from_json(const nlohmann::json& j, std::vector<DungeonRoom>& rooms) const
{
	if (!j.contains("rooms") || !j["rooms"].is_array())
		return;

	for (const auto& d : j["rooms"])
	{
		rooms.push_back(DungeonRoom{
			d.value("col", 0),
			d.value("row", 0),
			d.value("width", 1),
			d.value("height", 1) });
	}
}

void GameStateManager::save_creatures_to_json(const std::vector<std::unique_ptr<Creature>>& creatures, nlohmann::json& j) const
{
	j["creatures"] = json::array();
	for (const auto& creature : creatures)
	{
		if (creature)
		{
			json creatureJson;
			creature->save(creatureJson);
			j["creatures"].push_back(creatureJson);
		}
	}
}

void GameStateManager::load_creatures_from_json(const nlohmann::json& j, std::vector<std::unique_ptr<Creature>>& creatures) const
{
	if (j.contains("creatures") && j["creatures"].is_array())
	{
		for (const auto& creatureData : j["creatures"])
		{
			auto creature = std::make_unique<Creature>(Vector2D{ 0, 0 }, ActorData{ 0, "Unnamed", WHITE_BLACK_PAIR });
			creature->load(creatureData);
			creatures.push_back(std::move(creature));
		}
	}
}
