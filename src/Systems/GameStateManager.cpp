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
#include "../Actor/InventoryOperations.h"
#include "../ActorTypes/Player.h"
#include "../Colors/Colors.h"
#include "../Core/GameContext.h"
#include "../Core/Paths.h"
#include "../Gui/Gui.h"
#include "../Map/DungeonRoom.h"
#include "../Map/Map.h"
#include "../Renderer/Renderer.h"
#include "../Systems/DataManager.h"
#include "../Systems/HungerSystem.h"
#include "../Systems/LevelManager.h"
#include "../Systems/MenuManager.h"
#include "../Systems/MessageSystem.h"
#include "../Utils/Vector2D.h"
#include "ContentRegistry.h"
#include "ContentRegistryIO.h"
#include "GameStateManager.h"
#include "TileConfig.h"

using json = nlohmann::json;
using namespace InventoryOperations;

namespace
{

void save_rooms(const std::vector<DungeonRoom>& rooms, json& j)
{
	j["rooms"] = json::array();
	for (const auto& room : rooms)
	{
		j["rooms"].push_back({ { "col", room.col },
			{ "row", room.row },
			{ "width", room.width },
			{ "height", room.height },
			{ "type", static_cast<int>(room.type) } });
	}
}

void load_rooms(const json& j, std::vector<DungeonRoom>& rooms)
{
	if (!j.contains("rooms") || !j["rooms"].is_array())
	{
		return;
	}

	for (const auto& d : j["rooms"])
	{
		DungeonRoom room;
		room.col = d.value("col", 0);
		room.row = d.value("row", 0);
		room.width = d.value("width", 1);
		room.height = d.value("height", 1);
		room.type = static_cast<RoomType>(d.value("type", static_cast<int>(RoomType::STANDARD)));
		rooms.push_back(std::move(room));
	}
}

void save_creatures(const std::vector<std::unique_ptr<Creature>>& creatures, json& j)
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

void load_creatures(const json& j, std::vector<std::unique_ptr<Creature>>& creatures)
{
	if (j.contains("creatures") && j["creatures"].is_array())
	{
		for (const auto& creatureData : j["creatures"])
		{
			auto creature = std::make_unique<Creature>(Vector2D{ 0, 0 }, ActorData{ TileRef{}, "Unnamed", WHITE_BLACK_PAIR });
			creature->load(creatureData);
			creatures.push_back(std::move(creature));
		}
	}
}

} // namespace

void GameStateManager::init_new_game(GameContext& ctx)
{
	assert(ctx.dataManager != nullptr);
	assert(ctx.messageSystem != nullptr);
	assert(ctx.levelManager != nullptr);
	assert(ctx.gameState != nullptr);
	assert(ctx.map != nullptr);
	assert(ctx.playerOwner != nullptr);
	assert(ctx.tileConfig != nullptr);

	ContentRegistryIO::load(*ctx.contentRegistry, Paths::CONTENT_TILES);
	ctx.dataManager->load_all_data(*ctx.messageSystem);

	ctx.levelManager->reset_to_first_level();
	ctx.gameState->set_time(0);
	ctx.gameState->set_is_loaded_game(false);

	assert(ctx.playerBlueprint != nullptr);
	*ctx.playerOwner = std::make_unique<Player>(Vector2D{ 0, 0 }, *ctx.playerBlueprint, ctx);
	*ctx.playerBlueprint = PlayerBlueprint{};
	ctx.player = ctx.playerOwner->get();
	ctx.player->actorData.tile = ctx.tileConfig->get("TILE_PLAYER");

	ctx.map->regenerate(ctx);

	ctx.gameState->set_game_status(GameStatus::STARTUP);
	ctx.messageSystem->log("New game initialized");
}

bool GameStateManager::load_all(GameContext& ctx)
{
	assert(ctx.menuManager != nullptr);
	assert(ctx.dataManager != nullptr);
	assert(ctx.messageSystem != nullptr);
	assert(ctx.gameState != nullptr);
	assert(ctx.playerOwner != nullptr);
	assert(ctx.tileConfig != nullptr);

	ContentRegistryIO::load(*ctx.contentRegistry, Paths::CONTENT_TILES);
	ctx.menuManager->set_game_initialized(true);
	ctx.dataManager->load_all_data(*ctx.messageSystem);

	*ctx.playerOwner = std::make_unique<Player>(Vector2D{ 0, 0 });
	ctx.player = ctx.playerOwner->get();
	ctx.player->actorData.tile = ctx.tileConfig->get("TILE_PLAYER");

	if (!load_game(ctx))
	{
		ctx.messageSystem->log("Error: Could not open save file.");
		return false;
	}

	ctx.gameState->set_is_loaded_game(true);
	ctx.gameState->set_game_status(GameStatus::STARTUP);
	ctx.messageSystem->log("GameStatus set to STARTUP after loading for FOV computation");
	return true;
}

void GameStateManager::save_game(GameContext& ctx)
{
	assert(ctx.map != nullptr);
	assert(ctx.rooms != nullptr);
	assert(ctx.player != nullptr);
	assert(ctx.stairs != nullptr);
	assert(ctx.creatures != nullptr);
	assert(ctx.inventoryData != nullptr);
	assert(ctx.gui != nullptr);
	assert(ctx.hungerSystem != nullptr);
	assert(ctx.levelManager != nullptr);
	assert(ctx.gameState != nullptr);

	auto save_path = Paths::resolve(Paths::SAVE_FILE);
	std::filesystem::create_directories(save_path.parent_path());

	std::filesystem::path temp_path = save_path;
	temp_path.replace_extension(".tmp");

	{
		std::ofstream file(temp_path);
		if (!file.is_open())
		{
			throw std::runtime_error("Error occurred while saving the game.");
		}

		json j;

		ctx.map->save(j);
		save_rooms(*ctx.rooms, j);

		json playerJson;
		ctx.player->save(playerJson);
		j["player"] = playerJson;

		json stairsJson;
		ctx.stairs->save(stairsJson);
		j["stairs"] = stairsJson;

		save_creatures(*ctx.creatures, j);
		save_inventory(*ctx.inventoryData, j);

		json guiJson;
		ctx.gui->save(guiJson);
		j["gui"] = guiJson;

		json hungerJson;
		ctx.hungerSystem->save(hungerJson);
		j["hunger_system"] = hungerJson;

		ctx.levelManager->save_to_json(j);
		j["time"] = ctx.gameState->get_time();

		file << j.dump(4);
	}

	std::filesystem::rename(temp_path, save_path);
}

bool GameStateManager::load_game(GameContext& ctx)
{
	assert(ctx.map != nullptr);
	assert(ctx.rooms != nullptr);
	assert(ctx.player != nullptr);
	assert(ctx.stairs != nullptr);
	assert(ctx.creatures != nullptr);
	assert(ctx.inventoryData != nullptr);
	assert(ctx.gui != nullptr);
	assert(ctx.hungerSystem != nullptr);
	assert(ctx.levelManager != nullptr);
	assert(ctx.gameState != nullptr);

	std::ifstream file(Paths::resolve(Paths::SAVE_FILE));
	if (!file.is_open())
	{
		return false;
	}

	json j;
	file >> j;

	ctx.map->load(j);
	load_rooms(j, *ctx.rooms);

	if (j.contains("player"))
	{
		ctx.player->load(j["player"]);
	}

	if (j.contains("stairs"))
	{
		ctx.stairs->load(j["stairs"]);
	}

	load_creatures(j, *ctx.creatures);
	load_inventory(*ctx.inventoryData, j);

	if (j.contains("gui"))
	{
		ctx.gui->load(j["gui"]);
	}

	if (j.contains("hunger_system"))
	{
		ctx.hungerSystem->load(ctx, j["hunger_system"]);
	}

	ctx.levelManager->load_from_json(j);

	if (j.contains("time"))
	{
		ctx.gameState->set_time(j["time"]);
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
	{
		return true; // File doesn't exist, nothing to delete
	}

	const bool removed = std::filesystem::remove(save_path, ec);
	return removed && !ec;
}
