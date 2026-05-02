#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include <raylib.h>

#include "../Ai/Ai.h"
#include "../Colors/Colors.h"
#include "../Core/GameContext.h"
#include "../Map/Map.h"
#include "../Persistent/Persistent.h"
#include "../Renderer/Renderer.h"
#include "../Systems/TileConfig.h"
#include "../Utils/UniqueId.h"
#include "../Utils/Vector2D.h"
#include "Actor.h"
#include "Item.h"

Actor::Actor(Vector2D position, ActorData data)
	: position(position),
	  actorData(data),
	  uniqueId(UniqueId::Generator::generate())
{
}

bool Actor::has_state(ActorState state) const noexcept
{
	// TODO: replace with std::ranges::contains
	return std::ranges::find(states, state) != states.end();
}

void Actor::add_state(ActorState state) noexcept
{
	states.push_back(state);
}

void Actor::remove_state(ActorState state) noexcept
{
	std::erase_if(states,
		[state](ActorState s)
		{
			return s == state;
		});
}

void Actor::load(const json& j)
{
	position.x = j["position"]["x"];
	position.y = j["position"]["y"];
	direction.x = j["direction"]["x"];
	direction.y = j["direction"]["y"];
	const auto& tileJ = j["actorData"].at("tile");
	actorData.tile = TileRef{
		static_cast<TileSheet>(tileJ.at("sheet").get<int>()),
		tileJ.at("col").get<int>(),
		tileJ.at("row").get<int>()
	};
	actorData.name = j["actorData"].at("name").get<std::string>();
	actorData.color = j["actorData"].at("color").get<int>();
	uniqueId = j.at("uniqueId").get<UniqueId::IdType>();

	// Clear existing states before loading saved states
	states.clear();

	// Deserialize vector of states
	for (const auto& state : j["states"])
	{
		states.push_back(state);
	}
}

void Actor::save(json& j)
{
	j["position"] = { { "y", position.y }, { "x", position.x } };
	j["direction"] = { { "y", direction.y }, { "x", direction.x } };
	j["actorData"] = {
		{ "tile", { { "sheet", static_cast<int>(actorData.tile.sheet) }, { "col", actorData.tile.col }, { "row", actorData.tile.row } } },
		{ "name", actorData.name },
		{ "color", actorData.color }
	};
	j["uniqueId"] = uniqueId;

	// Serialize vector of states
	json statesJson;
	for (const auto& state : states)
	{
		statesJson.push_back(state);
	}
	j["states"] = statesJson;
}

// a function to get the Chebyshev distance from an actor to a specific tile of the map
int Actor::get_tile_distance(Vector2D tilePosition) const noexcept
{
	return std::max(std::abs(position.x - tilePosition.x), std::abs(position.y - tilePosition.y));
}

// the actor render function with color
void Actor::render(const GameContext& ctx) const noexcept
{
	if (is_visible(ctx) && ctx.renderer)
	{
		// Now it naturally resolves whether it's an Actor, Creature, or Item
		TileRef displayTile = get_display_tile();
		ctx.renderer->draw_tile(position, displayTile, Color{ 255, 255, 255, 255 });
	}
}

// check if the actor is visible
bool Actor::is_visible(const GameContext& ctx) const noexcept
{
	// Invisible actors are hidden unless there's special detection
	if (has_state(ActorState::IS_INVISIBLE))
	{
		return false;
	}

	return (!has_state(ActorState::FOV_ONLY) && ctx.map->is_explored(position)) || ctx.map->is_in_fov(position);
}
