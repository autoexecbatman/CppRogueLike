#pragma once
#include "Ai.h"

class TCODZip;
class Actor;
class Creature;
class Player;
class Item;
class Vector2D;
enum class Controls;

class AiPlayer : public Ai
{
public:
	void update(Creature& owner) override;
	void load(const json& j) override;
	void save(json& j) override;
	void display_inventory(Player& player);
	void applyConfusion(int duration) { confusionTurns = duration; }
	bool isConfused() const { return confusionTurns > 0; }
private:
	bool shouldComputeFOV{ false };
	bool isWaiting{ false };
	int confusionTurns = 0;  // Number of turns player remains confused

	void move(Creature& owner, Vector2D target);
	void pick_item(Player& player);
	void drop_item(Player& player);
	bool is_pickable_at_position(const Actor& actor, const Actor& owner) const;
	void display_inventory_items(WINDOW* inv, const Player& player) noexcept;
	Item* chose_from_inventory(Player& player, int ascii);
	void look_on_floor(Vector2D target);
	bool look_to_attack(Vector2D& target, Creature& owner);
	void look_to_move(Creature& owner, const Vector2D& targetPosition);
	void call_action(Player& player, Controls key);
	Vector2D handle_direction_input(const Creature& owner, int dirKey);
};
