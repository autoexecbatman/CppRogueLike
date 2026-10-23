#include <algorithm>
#include <cassert>
#include <optional>
#include <string>
#include <vector>

#include <raylib.h>

#include "Actor.h"
#include "GameContext.h"
#include "Map.h"
#include "Persistent.h"
#include "Renderer.h"
#include "SavedEnum.h"
#include "TileRefJson.h"
#include "UniqueId.h"
#include "Vector2D.h"

namespace
{
// The ActorState a saved integer names, or nullopt for one no state has.
// The switch has no default, so a state added without a case here is a compile
// error (C4062, -Wswitch).
//
// Example:
//   actor_state_from(10);    // -> ActorState::IS_SURPRISED
//   actor_state_from(11);    // -> nullopt
std::optional<ActorState> actor_state_from(int saved)
{
	const ActorState candidate = static_cast<ActorState>(saved);
	switch (candidate)
	{
	case ActorState::BLOCKS:
	case ActorState::FOV_ONLY:
	case ActorState::CAN_SWIM:
	case ActorState::CAN_FLY:
	case ActorState::IS_INVISIBLE:
	case ActorState::IS_SLEEPING:
	case ActorState::IS_HELD:
	case ActorState::IS_SILENCED:
	case ActorState::IS_FLEEING:
	case ActorState::CAN_WALK_WEBS:
	case ActorState::IS_SURPRISED:
	{
		return candidate;
	}
	}
	return std::nullopt;
}
} // namespace

Actor::Actor(Vector2D position, ActorData data) :
	position(position),
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
	std::erase_if(
		states,
		[state](ActorState existing) { return existing == state; });
}

void Actor::load(const json& source)
{
	// Actor::save writes every key read here, so each is required: a missing one throws.
	position.x = source.at("position").at("x").get<int>();
	position.y = source.at("position").at("y").get<int>();
	direction.x = source.at("direction").at("x").get<int>();
	direction.y = source.at("direction").at("y").get<int>();
	actorData.tile = TileRefJson::parse_tile(source.at("actorData").at("tile"));
	actorData.name = source.at("actorData").at("name").get<std::string>();
	actorData.color =
		parse_color_pair(source.at("actorData").at("color").get<std::string>());
	uniqueId = source.at("uniqueId").get<UniqueId::IdType>();

	// Clear existing states before loading saved states
	states.clear();

	// An actor with no states was saved with null here, which iterates as empty.
	for (const auto& state : source.at("states"))
	{
		states.push_back(saved_enum_value(
			state,
			"Actor::load",
			"states",
			"ActorState",
			&actor_state_from));
	}
}

void Actor::save(json& output)
{
	output["position"] = { { "y", position.y }, { "x", position.x } };
	output["direction"] = { { "y", direction.y }, { "x", direction.x } };
	output["actorData"] = {
		{
			"tile",
			TileRefJson::encode_tile(actorData.tile),
		},
		{ "name", actorData.name },
		{ "color", color_pair_name(actorData.color) },
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
		ctx.renderer->draw_tile(
			position,
			displayTile,
			Color{ 255, 255, 255, 255 });
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

	const bool isRemembered =
		!has_state(ActorState::FOV_ONLY) && ctx.map->is_explored(position);
	return isRemembered || ctx.map->is_in_fov(position);
}
