#include <algorithm>
#include <cassert>
#include <string>
#include <vector>

#include <raylib.h>

#include "Actor.h"
#include "GameContext.h"
#include "Map.h"
#include "Persistent.h"
#include "Renderer.h"
#include "TileRefJson.h"
#include "UniqueId.h"
#include "Vector2D.h"

Actor::Actor(Vector2D position, ActorData data)
	: position(position),
	  actorData(data),
	  uniqueId(UniqueId::Generator::generate())
{
}

bool Actor::has_state(ActorState state) const noexcept
{
	return std::ranges::contains(states, state);
}

void Actor::add_state(ActorState state) noexcept
{
	states.push_back(state);
}

void Actor::remove_state(ActorState state) noexcept
{
	std::erase_if(states,
		[state](ActorState existing)
		{
			return existing == state;
		});
}

void Actor::load(const json& source)
{
	position.x = source["position"]["x"];
	position.y = source["position"]["y"];
	direction.x = source["direction"]["x"];
	direction.y = source["direction"]["y"];
	actorData.tile = TileRefJson::parse_tile(source["actorData"].at("tile"));
	actorData.name = source["actorData"].at("name").get<std::string>();
	actorData.color = parse_color_pair(source["actorData"].at("color").get<std::string>());
	uniqueId = source.at("uniqueId").get<UniqueId::IdType>();

	// Clear existing states before loading saved states
	states.clear();

	// Deserialize vector of states
	for (const auto& state : source["states"])
	{
		states.push_back(state);
	}
}

void Actor::save(json& output)
{
	output["position"] = { { "y", position.y }, { "x", position.x } };
	output["direction"] = { { "y", direction.y }, { "x", direction.x } };
	output["actorData"] = {
		{ "tile", TileRefJson::encode_tile(actorData.tile) },
		{ "name", actorData.name },
		{ "color", color_pair_name(actorData.color) }
	};
	output["uniqueId"] = uniqueId;

	// Serialize vector of states
	json statesJson;
	for (const auto& state : states)
	{
		statesJson.push_back(state);
	}
	output["states"] = statesJson;
}

// a function to get the Chebyshev distance from an actor to a specific tile of the map
int Actor::get_tile_distance(Vector2D tilePosition) const noexcept
{
	return position.chebyshev_distance_to(tilePosition);
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
	assert(ctx.map && "Actor::is_visible: called without a map");

	// Invisible actors are hidden unless there's special detection
	if (has_state(ActorState::IS_INVISIBLE))
	{
		return false;
	}

	return (!has_state(ActorState::FOV_ONLY) && ctx.map->is_explored(position)) || ctx.map->is_in_fov(position);
}
