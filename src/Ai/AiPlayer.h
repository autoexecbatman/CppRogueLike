#pragma once
#include "Ai.h"

class TCODZip;
class Actor;
class Creature;
class Item;
class Vector2D;
enum class Controls;

class AiPlayer : public Ai
{
public:
	void update(Creature& owner) override;
	void load(const json& j) override;
	void save(json& j) override;
	void display_inventory(Creature& owner);
private:
	bool shouldComputeFOV{ false };
	bool isWaiting{ false };

	void move(Creature& owner, Vector2D target);
	void pick_item(Creature& owner);
	void drop_item(Creature& owner);
	bool is_pickable_at_position(const Actor& actor, const Actor& owner) const;
	void display_inventory_items(WINDOW* inv, const Creature& owner) noexcept;
	Item* chose_from_inventory(Creature& owner, int ascii);
	void look_on_floor(Vector2D target);
	bool look_to_attack(Vector2D& target, Creature& owner);
	void look_to_move(Creature& owner, const Vector2D& targetPosition);
	void call_action(Creature& owner, Controls key);
};
