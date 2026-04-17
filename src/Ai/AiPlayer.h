#pragma once

#include <optional>
#include <vector>

#include "../Core/GameContext.h"
#include "../Persistent/Persistent.h"
#include "Ai.h"

class TCODZip;
class Actor;
class Creature;
class Player;
class Item;
struct Vector2D;

enum class Controls;

enum class PendingDoorAction
{
	NONE,
	OPEN,
	CLOSE
};

class AiPlayer final : public Ai
{
private:
	// Mouse navigation mode — one authoritative state, no scattered optionals
	enum class MouseMode
	{
		IDLE,
		WALK, // plain A* walk, no arrival action
		WALK_TO_PICKUP, // walk then pick up item
		WALK_TO_DOOR, // walk then open/close door
		WALK_TO_STAIRS // walk then descend
	};

	bool shouldComputeFOV{ false };
	bool isWaiting{ false };
	int confusionTurns{ 0 };
	PendingDoorAction pendingDoorAction{ PendingDoorAction::NONE }; // keyboard door prompt
	MouseMode mouseMode{ MouseMode::IDLE };
	PendingDoorAction mouseDoorAction{ PendingDoorAction::NONE }; // WALK_TO_DOOR intent
	Vector2D mouseDoorTarget{ -1, -1 }; // destination tile for WALK_TO_DOOR

	void move(Creature& owner, Vector2D target);
	void pick_item(Player& player, GameContext& ctx);
	void drop_item(Player& player, GameContext& ctx);
	bool is_pickable_at_position(const Actor& actor, const Actor& owner) const;
	void display_inventory_items(void* inv, const Player& player) noexcept;
	Item* chose_from_inventory(Player& player, int ascii, GameContext& ctx);
	void look_on_floor(Vector2D target, GameContext& ctx);
	bool look_to_attack(Vector2D& target, Creature& owner, GameContext& ctx);
	bool look_to_move(Creature& owner, const Vector2D& targetPosition, GameContext& ctx);
	void call_action(Player& player, Controls key, GameContext& ctx);
	bool resolve_pending_door(Creature& owner, GameContext& ctx);
	Vector2D handle_direction_input(const Creature& owner, int dirKey, GameContext& ctx);
	bool resolve_mouse_world_tile(GameContext& ctx, Vector2D& out_world_tile) const;
	void flush_fov(GameContext& ctx);
	bool is_mouse_pending_cancelled(GameContext& ctx) const;
	bool handle_mouse_path(Creature& owner, GameContext& ctx);
	bool execute_arrival(Creature& owner, GameContext& ctx);
	void handle_left_click(Player& player, GameContext& ctx);
	void handle_right_click(Player& player, GameContext& ctx);
	Vector2D find_door_approach(Vector2D doorTile, const GameContext& ctx) const;
	void begin_path_walk(
		Vector2D walkDest,
		Vector2D actionTarget,
		MouseMode mode,
		PendingDoorAction doorAction,
		GameContext& ctx);

public:
	void update(Creature& owner, GameContext& ctx) override;
	void load(const json& j) override;
	void save(json& j) override;
	void display_inventory(Player& player, GameContext& ctx);
	void apply_confusion(int duration) override { confusionTurns = duration; }
	bool is_confused() const { return confusionTurns > 0; }

};
