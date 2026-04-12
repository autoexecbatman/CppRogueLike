#pragma once

#include <string>
#include <vector>

#include "../Colors/Colors.h"
#include "../Persistent/Persistent.h"
#include "../Renderer/Renderer.h"
#include "../Utils/UniqueId.h"
#include "../Utils/Vector2D.h"

struct GameContext;

struct ActorData
{
	TileRef tile{};
	std::string name{ "string" };
	int color{ WHITE_BLACK_PAIR };
};

enum class ActorState
{
	BLOCKS,
	FOV_ONLY,
	CAN_SWIM,
	IS_EQUIPPED,
	IS_RANGED,
	IS_CONFUSED,
	IS_INVISIBLE,
	IS_SLEEPING,
	IS_HELD,
	IS_PROTECTED,
	IS_SILENCED,
	IS_FLEEING
};

//==Actor==
// a class for the actors in the game
// (player, monsters, items, etc.)
class Actor : public Persistent
{
public:
	Vector2D position{ 0, 0 };
	Vector2D direction{ 0, 0 };
	ActorData actorData{ TileRef{}, "string", 0 };
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

	void load(const json& j) override;
	void save(json& j) override;

	int get_tile_distance(Vector2D tilePosition) const noexcept;
	void render(const GameContext& ctx) const noexcept;
	bool is_visible(const GameContext& ctx) const noexcept;

	[[nodiscard]] std::string_view get_name() const { return actorData.name; }

	virtual TileRef get_display_tile() const noexcept
	{
		return actorData.tile;
	}

	virtual int get_display_color() const noexcept
	{
		return actorData.color;
	}
};
