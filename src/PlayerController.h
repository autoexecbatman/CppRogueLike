// file: PlayerController.h
// Translates human input (keyboard + mouse) into game actions for the player.
// Not an AI -- does not inherit from Ai. Player owns this; monsters never touch it.
#pragma once

#include "GameContext.h"
#include "Persistent.h"
#include "TurnSchedule.h"

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
	// The clock reading the confusion ends at. A count of rounds would fall once per
	// action, so a hasted player would shake the spell off in half the rounds it named.
	int confusionEndTime{ 0 };
	PendingDoorAction pendingDoorAction{ PendingDoorAction::NONE };
	MouseMode mouseMode{ MouseMode::IDLE };
	PendingDoorAction mouseDoorAction{ PendingDoorAction::NONE };
	Vector2D mouseDoorTarget{ -1, -1 };

	void move(Vector2D target);
	// Picks up the first item lying on the player's own tile. Gold goes straight to the
	// purse and is never carried. Anything else is refused before ownership moves - a
	// full pack says so, and one that would be overloaded says "Too heavy to carry" -
	// so a refused item stays on the floor rather than being destroyed.
	//
	// Example, standing on a potion with room to carry it:
	//   pick_item(ctx);   // "You picked up a health potion." - the floor loses it
	//   pick_item(ctx);   // "There's nothing here to pick up." - the tile is bare now
	void pick_item(GameContext& ctx);
	void drop_item(GameContext& ctx);
	bool is_pickable_at_position(const Actor& actor) const;
	Item* chose_from_inventory(int ascii, GameContext& ctx);
	// Names every item lying on one tile, one message each. Reports nothing when the
	// tile is bare - it is the look command's floor half, so silence is the answer for
	// an empty tile rather than a message saying so.
	//
	// Example, a tile holding a sword and a potion:
	//   look_on_floor(tile, ctx);   // "There's a long sword here", "There's a health potion here"
	void look_on_floor(Vector2D target, GameContext& ctx);
	bool look_to_attack(Vector2D& target, GameContext& ctx);
	bool look_to_move(const Vector2D& targetPosition, GameContext& ctx);
	void call_action(Controls key, GameContext& ctx);
	bool resolve_pending_door(GameContext& ctx);
	void resolve_peaceful_bump(Creature& target, GameContext& ctx);
	void swap_places_with(Creature& target, GameContext& ctx);
	void attempt_turn_undead(GameContext& ctx);
	void confirm_attack_on_peaceful(Creature& target, GameContext& ctx);
	void strike(Creature& target, GameContext& ctx);
	Vector2D handle_direction_input(int dirKey, GameContext& ctx);
	bool resolve_mouse_world_tile(GameContext& ctx, Vector2D& out_world_tile) const;
	void flush_fov(GameContext& ctx);
	bool is_mouse_pending_cancelled(GameContext& ctx) const;
	bool handle_mouse_path(GameContext& ctx);
	bool execute_arrival(GameContext& ctx);
	// Turns a left click on the map into an action, chosen by what is under it.
	// Clicking the player's own tile takes the stairs when standing on them and picks
	// up otherwise. Clicking an adjacent creature attacks it at once; a distant one is
	// not walked to, because A* is blocked by can_walk. Any other tile begins a path,
	// and a tile holding an item or a door walks to pick up or to open.
	//
	// Example, clicking a tile two squares away that holds a potion:
	//   handle_left_click(ctx);   // mode becomes WALK_TO_PICKUP and the path begins
	void handle_left_click(GameContext& ctx);
	// Opens a context menu for whatever is under the cursor, built from what is
	// actually there: an item on the tile adds "Pick up", a door adds its own actions.
	// A menu always opens, because "Cancel" is appended unconditionally - an empty tile
	// gets a menu offering only that.
	//
	// Example, right-clicking a tile holding a dagger:
	//   handle_right_click(ctx);   // a menu with "Pick up dagger" and "Cancel"
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
	void load(const json& savedState) override;
	void save(json& savedState) override;
	void display_inventory(GameContext& ctx);

	void apply_confusion(int durationRounds, int currentTime) { confusionEndTime = expiry_time(currentTime, durationRounds); }
	[[nodiscard]] bool is_confused(int currentTime) const { return currentTime < confusionEndTime; }
};
