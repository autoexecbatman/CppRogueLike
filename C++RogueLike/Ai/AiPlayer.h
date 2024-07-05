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
	void load(TCODZip& zip) override;
	void save(TCODZip& zip) override;
	void display_inventory(Creature& owner);
private:
	int get_next_level_xp(Creature& owner);
	void levelup_update(Creature& owner);
	void pick_item(Creature& owner);
	void drop_item(Creature& owner);
	bool is_pickable_at_position(const Actor& actor, const Actor& owner) const;
	void display_inventory_items(WINDOW* inv, const Creature& owner) noexcept;
	Item* chose_from_inventory(Creature& owner, int ascii);
	bool move_or_attack(Creature& owner, Vector2D target);
	void look_on_floor(Vector2D& target);
	bool look_to_attack(Vector2D& target, Creature& owner);
	void call_action(Creature& owner, Controls key);
};
