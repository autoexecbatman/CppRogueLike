// GameStateManager.cpp - Handles game state persistence and level management
#include <cassert>
#include <filesystem>
#include <format>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <system_error>
#include <utility>
#include <vector>

#include <nlohmann/json_fwd.hpp>

#include "Actor.h"
#include "BodyPlanRegistry.h"
#include "Colors.h"
#include "ContentRegistryIO.h"
#include "DataManager.h"
#include "Decoration.h"
#include "DungeonRoom.h"
#include "GameContext.h"
#include "GameStateManager.h"
#include "Gui.h"
#include "HungerSystem.h"
#include "InventoryOperations.h"
#include "LevelManager.h"
#include "Map.h"
#include "MenuManager.h"
#include "MessageSystem.h"
#include "Paths.h"
#include "Player.h"
#include "Renderer.h"
#include "SpellTile.h"
#include "TileConfig.h"
#include "TileRefJson.h"
#include "Trap.h"
#include "UniqueId.h"
#include "Vector2D.h"

using json = nlohmann::json;
using namespace InventoryOperations;

void save_rooms(const std::vector<DungeonRoom>& rooms, json& output)
{
	output["rooms"] = json::array();
	for (const auto& room : rooms)
	{
		output["rooms"].push_back({ { "col", room.col },
			{ "row", room.row },
			{ "width", room.width },
			{ "height", room.height },
			{ "type", static_cast<int>(room.type) } });
	}
}

void load_rooms(const json& source, std::vector<DungeonRoom>& rooms)
{
	// Replace rather than append, as every loader of a level container does.
	rooms.clear();

	const json& roomRecords = source.at("rooms");
	if (!roomRecords.is_array())
	{
		throw std::runtime_error(std::format("load_rooms: \"rooms\" is {}, not an array", roomRecords.type_name()));
	}

	for (const auto& roomRecord : roomRecords)
	{
		// A type read from disk is a boundary: TREASURE is RoomType's last value.
		const int type = roomRecord.at("type").get<int>();
		if (type < 0 || type > static_cast<int>(RoomType::TREASURE))
		{
			throw std::runtime_error(std::format("load_rooms: room type {} is not a RoomType", type));
		}

		DungeonRoom room;
		room.col = roomRecord.at("col").get<int>();
		room.row = roomRecord.at("row").get<int>();
		room.width = roomRecord.at("width").get<int>();
		room.height = roomRecord.at("height").get<int>();
		room.type = static_cast<RoomType>(type);
		rooms.push_back(std::move(room));
	}
}

namespace
{

// Writes every creature on the level into j["creatures"], in the order the vector holds
// them. The vector owns its entries, so a null one is a fault rather than an empty slot -
// skipping it would drop a creature from the save without saying so.
//
// Example:
//
//   json level;
//   save_creatures(creatures, level);   // creatures: vector<unique_ptr<Creature>>, 3 alive
//   level["creatures"].size();          // -> 3
//
//   save_creatures({}, level);          // an empty level is legal
//   level["creatures"].size();          // -> 0
void save_creatures(const std::vector<std::unique_ptr<Creature>>& creatures, json& output)
{
	output["creatures"] = json::array();
	for (const auto& creature : creatures)
	{
		assert(creature && "save_creatures: creatures list holds a null entry");

		json creatureJson;
		creature->save(creatureJson);
		output["creatures"].push_back(creatureJson);
	}
}

} // namespace

void load_creatures(const json& source, std::vector<std::unique_ptr<Creature>>& creatures)
{
	// Replace rather than append, as every loader of a level container does.
	creatures.clear();

	if (source.contains("creatures") && source["creatures"].is_array())
	{
		for (const auto& creatureData : source["creatures"])
		{
			auto creature = std::make_unique<Creature>(Vector2D{ 0, 0 }, ActorData{ TileRef{}, "Unnamed", ColorPairId::WHITE_BLACK });
			creature->load(creatureData);
			creatures.push_back(std::move(creature));
		}
	}
}

void save_decorations(const std::vector<std::unique_ptr<Decoration>>& decorations, json& output)
{
	output["decorations"] = json::array();
	for (const auto& decoration : decorations)
	{
		assert(decoration && "save_decorations: the decoration list holds a null entry");
		output["decorations"].push_back({ { "position", { { "x", decoration->position.x }, { "y", decoration->position.y } } },
			{ "tile", TileRefJson::encode_tile(decoration->tile) },
			{ "name", decoration->name },
			{ "hp", decoration->hp },
			{ "blocksMovement", decoration->blocks_movement },
			{ "lootTableKey", decoration->lootTableKey },
			{ "isBroken", decoration->isBroken } });
	}
}

void load_decorations(const json& source, std::vector<std::unique_ptr<Decoration>>& decorations)
{
	// Replace rather than append, as every loader of a level container does.
	decorations.clear();

	for (const auto& record : source.at("decorations"))
	{
		auto decoration = std::make_unique<Decoration>();
		decoration->position = Vector2D{ record.at("position").at("x").get<int>(), record.at("position").at("y").get<int>() };
		decoration->tile = TileRefJson::parse_tile(record.at("tile"));
		decoration->name = record.at("name").get<std::string>();
		decoration->hp = record.at("hp").get<int>();
		decoration->blocks_movement = record.at("blocksMovement").get<bool>();
		decoration->lootTableKey = record.at("lootTableKey").get<std::string>();
		decoration->isBroken = record.at("isBroken").get<bool>();
		decorations.push_back(std::move(decoration));
	}
}

void save_traps(const std::vector<std::unique_ptr<Trap>>& traps, json& output)
{
	output["traps"] = json::array();
	for (const auto& trap : traps)
	{
		assert(trap && "save_traps: the trap list holds a null entry");
		json trapJson;
		trap->save(trapJson);
		output["traps"].push_back(trapJson);
	}
}

void load_traps(const json& source, std::vector<std::unique_ptr<Trap>>& traps, const TileConfig& tileConfig)
{
	// Replace rather than append: a load during play would otherwise leave the
	// abandoned level's traps on the new map, at the tiles they had there.
	traps.clear();

	if (!source.contains("traps") || !source["traps"].is_array())
	{
		return;
	}

	for (const auto& trapJson : source["traps"])
	{
		// Built through the same constructor the generator uses, so the type decides
		// the dice, the name and both tiles exactly as it did the first time.
		const auto type = static_cast<TrapType>(trapJson.at("type").get<int>());
		auto trap = std::make_unique<Trap>(Vector2D{ 0, 0 }, type, tileConfig);
		trap->load(trapJson);
		traps.push_back(std::move(trap));
	}
}

void GameStateManager::init_new_game(GameContext& ctx)
{
	assert(ctx.bodyPlanRegistry && "GameStateManager::init_new_game: called without a bodyPlanRegistry");
	assert(ctx.contentRegistry && "GameStateManager::init_new_game: called without a contentRegistry");
	assert(ctx.dataManager != nullptr && "GameStateManager::init_new_game: called without a dataManager");
	assert(ctx.messageSystem != nullptr && "GameStateManager::init_new_game: called without a messageSystem");
	assert(ctx.levelManager != nullptr && "GameStateManager::init_new_game: called without a levelManager");
	assert(ctx.gameState != nullptr && "GameStateManager::init_new_game: called without a gameState");
	assert(ctx.map != nullptr && "GameStateManager::init_new_game: called without a map");
	assert(ctx.playerOwner != nullptr && "GameStateManager::init_new_game: called without a playerOwner");
	assert(ctx.tileConfig != nullptr && "GameStateManager::init_new_game: called without a tileConfig");

	ContentRegistryIO::load(*ctx.contentRegistry, Paths::CONTENT_TILES);
	ctx.dataManager->load_all_data(*ctx.messageSystem);

	ctx.levelManager->reset_to_first_level();
	ctx.gameState->set_time(0);
	ctx.gameState->set_is_loaded_game(false);

	assert(ctx.playerBlueprint != nullptr && "GameStateManager::init_new_game: called without a playerBlueprint");
	*ctx.playerOwner = std::make_unique<Player>(Vector2D{ 0, 0 }, *ctx.playerBlueprint, ctx);
	ctx.player()->set_body_plan(ctx.bodyPlanRegistry->get("humanoid"));
	*ctx.playerBlueprint = PlayerBlueprint{};
	ctx.player()->actorData.tile = ctx.tileConfig->get(ctx.player_concrete().sprite_tile_key());

	ctx.map->regenerate(ctx);

	ctx.gameState->begin_run();
	ctx.messageSystem->log("New game initialized");
}

bool GameStateManager::load_all(GameContext& ctx)
{
	assert(ctx.contentRegistry && "GameStateManager::load_all: called without a contentRegistry");
	assert(ctx.menuManager != nullptr && "GameStateManager::load_all: called without a menuManager");
	assert(ctx.dataManager != nullptr && "GameStateManager::load_all: called without a dataManager");
	assert(ctx.messageSystem != nullptr && "GameStateManager::load_all: called without a messageSystem");
	assert(ctx.gameState != nullptr && "GameStateManager::load_all: called without a gameState");
	assert(ctx.playerOwner != nullptr && "GameStateManager::load_all: called without a playerOwner");
	assert(ctx.tileConfig != nullptr && "GameStateManager::load_all: called without a tileConfig");

	ContentRegistryIO::load(*ctx.contentRegistry, Paths::CONTENT_TILES);
	ctx.menuManager->set_game_initialized(true);
	ctx.dataManager->load_all_data(*ctx.messageSystem);

	*ctx.playerOwner = std::make_unique<Player>(Vector2D{ 0, 0 });

	if (!load_game(ctx))
	{
		ctx.messageSystem->log("Error: Could not open save file.");
		return false;
	}

	// load_game restores the creature class, so the class sprite is only known now.
	ctx.player()->actorData.tile = ctx.tileConfig->get(ctx.player_concrete().sprite_tile_key());

	ctx.gameState->set_is_loaded_game(true);
	ctx.gameState->begin_run();
	ctx.messageSystem->log("GameStatus set to STARTUP after loading for FOV computation");
	return true;
}

void GameStateManager::save_game(GameContext& ctx)
{
	assert(ctx.map != nullptr && "GameStateManager::save_game: called without a map");
	assert(ctx.rooms != nullptr && "GameStateManager::save_game: called without rooms");
	assert(ctx.player() != nullptr && "GameStateManager::save_game: called without a player");
	assert(ctx.stairs != nullptr && "GameStateManager::save_game: called without stairs");
	assert(ctx.creatures != nullptr && "GameStateManager::save_game: called without creatures");
	assert(ctx.floorInventory != nullptr && "GameStateManager::save_game: called without a floorInventory");
	assert(ctx.gui != nullptr && "GameStateManager::save_game: called without a gui");
	assert(ctx.hungerSystem != nullptr && "GameStateManager::save_game: called without a hungerSystem");
	assert(ctx.levelManager != nullptr && "GameStateManager::save_game: called without a levelManager");
	assert(ctx.gameState != nullptr && "GameStateManager::save_game: called without a gameState");

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

		json saveDocument;

		ctx.map->save(saveDocument);
		save_rooms(*ctx.rooms, saveDocument);

		assert(ctx.traps && "save_game: the level was saved with no trap container");
		save_traps(*ctx.traps, saveDocument);
		assert(ctx.decorations && "save_game: the level was saved with no decoration container");
		save_decorations(*ctx.decorations, saveDocument);

		json playerJson;
		ctx.player()->save(playerJson);
		saveDocument["player"] = playerJson;

		json stairsJson;
		ctx.stairs->save(stairsJson);
		saveDocument["stairs"] = stairsJson;

		save_creatures(*ctx.creatures, saveDocument);
		save_inventory(*ctx.floorInventory, saveDocument);

		json guiJson;
		ctx.gui->save(guiJson);
		saveDocument["gui"] = guiJson;

		json hungerJson;
		ctx.hungerSystem->save(hungerJson);
		saveDocument["hunger_system"] = hungerJson;

		ctx.levelManager->save_to_json(saveDocument);
		saveDocument["time"] = ctx.gameState->get_time();
		saveDocument["roundsRun"] = ctx.gameState->get_rounds_run();

		// Last, because every save above may still have been issuing ids.
		UniqueId::save(saveDocument);

		file << saveDocument.dump(4);
	}

	std::filesystem::rename(temp_path, save_path);
}

bool GameStateManager::load_game(GameContext& ctx)
{
	assert(ctx.map != nullptr && "GameStateManager::load_game: called without a map");
	assert(ctx.rooms != nullptr && "GameStateManager::load_game: called without rooms");
	assert(ctx.player() != nullptr && "GameStateManager::load_game: called without a player");
	assert(ctx.stairs != nullptr && "GameStateManager::load_game: called without stairs");
	assert(ctx.creatures != nullptr && "GameStateManager::load_game: called without creatures");
	assert(ctx.floorInventory != nullptr && "GameStateManager::load_game: called without a floorInventory");
	assert(ctx.gui != nullptr && "GameStateManager::load_game: called without a gui");
	assert(ctx.hungerSystem != nullptr && "GameStateManager::load_game: called without a hungerSystem");
	assert(ctx.levelManager != nullptr && "GameStateManager::load_game: called without a levelManager");
	assert(ctx.gameState != nullptr && "GameStateManager::load_game: called without a gameState");

	std::ifstream file(Paths::resolve(Paths::SAVE_FILE));
	if (!file.is_open())
	{
		return false;
	}

	json saveDocument;
	file >> saveDocument;

	ctx.map->load(saveDocument);
	load_rooms(saveDocument, *ctx.rooms);

	assert(ctx.traps && "load_game: a level was loaded with no trap container");
	assert(ctx.tileConfig && "load_game: a trap cannot be rebuilt without the tile table");
	load_traps(saveDocument, *ctx.traps, *ctx.tileConfig);
	assert(ctx.decorations && "load_game: a level was loaded with no decoration container");
	load_decorations(saveDocument, *ctx.decorations);

	// Spell tiles are not saved, so a load clears them: the abandoned level's would
	// otherwise stay at their old coordinates, which a fresh launch never shows.
	assert(ctx.spellTiles && "load_game: a level was loaded with no spell tile container");
	ctx.spellTiles->clear();

	if (saveDocument.contains("player"))
	{
		ctx.player()->load(saveDocument["player"]);
	}

	if (saveDocument.contains("stairs"))
	{
		ctx.stairs->load(saveDocument["stairs"]);
	}

	load_creatures(saveDocument, *ctx.creatures);

	// After every actor is built, because building one issues an id that its own
	// saved id then overwrites - so the counter has to be put back afterwards.
	UniqueId::load(saveDocument);
	load_inventory(*ctx.floorInventory, saveDocument);

	if (saveDocument.contains("gui"))
	{
		ctx.gui->load(saveDocument["gui"]);
	}

	if (saveDocument.contains("hunger_system"))
	{
		ctx.hungerSystem->load(saveDocument["hunger_system"]);
	}

	ctx.levelManager->load_from_json(saveDocument);

	if (saveDocument.contains("roundsRun"))
	{
		ctx.gameState->set_rounds_run(saveDocument["roundsRun"]);
	}

	if (saveDocument.contains("time"))
	{
		ctx.gameState->set_time(saveDocument["time"]);
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
	std::error_code error;
	auto save_path = Paths::resolve(Paths::SAVE_FILE);

	if (!std::filesystem::exists(save_path, error))
	{
		return true; // File doesn't exist, nothing to delete
	}

	const bool removed = std::filesystem::remove(save_path, error);
	return removed && !error;
}
