// file: PlayerController.h
// Translates human input (keyboard + mouse) into game actions for the player.
// Not an AI -- does not inherit from Ai. Player owns this; monsters never touch it.
#pragma once

#include "../Core/GameContext.h"
#include "../Persistent/Persistent.h"

class Player;
class Item;
struct Vector2D;

enum class Controls;

enum class PendingDoorAction
{
	NONE,
	OPEN,
	CLOSE,
	DISARM
};

class PlayerController final : public Persistent
{
private:
	Player& playerOwner;

	// Mouse navigation mode -- one authoritative state, no scattered optionals
	enum class MouseMode
	{
		IDLE,
		WALK,
		WALK_TO_PICKUP,
		WALK_TO_DOOR,
		WALK_TO_STAIRS
	};

	bool shouldComputeFOV{ false };
	bool isWaiting{ false };
	int confusionTurns{ 0 };
	PendingDoorAction pendingDoorAction{ PendingDoorAction::NONE };
	MouseMode mouseMode{ MouseMode::IDLE };
	PendingDoorAction mouseDoorAction{ PendingDoorAction::NONE };
	Vector2D mouseDoorTarget{ -1, -1 };

	void move(Vector2D target);
	void pick_item(GameContext& ctx);
	void drop_item(GameContext& ctx);
	bool is_pickable_at_position(const Actor& actor) const;
	Item* chose_from_inventory(int ascii, GameContext& ctx);
	void look_on_floor(Vector2D target, GameContext& ctx);
	bool look_to_attack(Vector2D& target, GameContext& ctx);
	bool look_to_move(const Vector2D& targetPosition, GameContext& ctx);
	void call_action(Controls key, GameContext& ctx);
	bool resolve_pending_door(GameContext& ctx);
	Vector2D handle_direction_input(int dirKey, GameContext& ctx);
	bool resolve_mouse_world_tile(GameContext& ctx, Vector2D& out_world_tile) const;
	void flush_fov(GameContext& ctx);
	bool is_mouse_pending_cancelled(GameContext& ctx) const;
	bool handle_mouse_path(GameContext& ctx);
	bool execute_arrival(GameContext& ctx);
	void handle_left_click(GameContext& ctx);
	void handle_right_click(GameContext& ctx);
	Vector2D find_door_approach(Vector2D doorTile, const GameContext& ctx) const;
	// Four-branch locked-door resolution: key, Open Locks, bash, blocked.
	// Returns true when a turn is consumed.
	bool resolve_locked_door(Vector2D doorPos, GameContext& ctx);
	void begin_path_walk(
		Vector2D walkDest,
		Vector2D actionTarget,
		MouseMode mode,
		PendingDoorAction doorAction,
		GameContext& ctx);

public:
	explicit PlayerController(Player& owner);

	void update(GameContext& ctx);
	void load(const json& j) override;
	void save(json& j) override;
	void display_inventory(GameContext& ctx);
	void apply_confusion(int duration) { confusionTurns = duration; }
	bool is_confused() const { return confusionTurns > 0; }
};
