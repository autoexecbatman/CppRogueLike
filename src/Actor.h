#pragma once

#include <string>
#include <vector>

#include <nlohmann/json_fwd.hpp>

#include "Colors.h"
#include "TileRef.h"
#include "UniqueId.h"
#include "Vector2D.h"

namespace CppRL
{

	struct GameContext;

	struct ActorData
	{
		TileRef tile{};
		std::string name{ "string" };
		ColorPairId color{ ColorPairId::WHITE_BLACK };
	};

	enum class ActorState
	{
		BLOCKS,
		FOV_ONLY,
		CAN_SWIM,
		CAN_FLY,
		IS_INVISIBLE,
		IS_SLEEPING,
		IS_HELD,
		IS_SILENCED,
		IS_FLEEING,
		CAN_WALK_WEBS,
		IS_SURPRISED
	};

	//==Actor==
	// a class for the actors in the game
	// (player, monsters, items, etc.)
	class Actor
	{
	public:
		Vector2D position{ 0, 0 };
		Vector2D direction{ 0, 0 };
		ActorData actorData{ TileRef{}, "string", ColorPairId::WHITE_BLACK };
		UniqueId::IdType uniqueId{};
		std::vector<ActorState> states;

		Actor(Vector2D position, ActorData data);
		virtual ~Actor() = default;
		Actor(const Actor&) = delete;
		Actor& operator=(const Actor&) = delete;
		Actor(Actor&&) = delete;
		Actor& operator=(Actor&&) = delete;

		// C++ Core Guidelines F.6: noexcept for simple state operations
		bool has_state(ActorState state) const noexcept;
		void add_state(ActorState state) noexcept;
		void remove_state(ActorState state) noexcept;

		// Restores what save wrote. Every key is required, so a missing one
		// throws. Creature, Item and Trap call it before reading their own.
		virtual void load(const nlohmann::json& source);
		// Writes position, direction, tile, name, color, id and states.
		virtual void save(nlohmann::json& output) const;

		int get_tile_distance(Vector2D tilePosition) const noexcept;
		void render(const GameContext& ctx) const noexcept;
		bool is_visible(const GameContext& ctx) const noexcept;

		virtual TileRef get_display_tile() const noexcept
		{
			return actorData.tile;
		}

		virtual ColorPairId get_display_color() const noexcept
		{
			return actorData.color;
		}
	};

} // namespace CppRL
