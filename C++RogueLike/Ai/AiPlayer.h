// file: Ai.h
#ifndef AIPLAYER_H
#define AIPLAYER_H

#pragma once

#include <curses.h>
#include <libtcod.h>

#include "../Actor/Actor.h"
#include "Ai.h"
#include "../Controls/Controls.h"

//==PLAYER_AI==
class AiPlayer : public Ai
{
public:
	void update(Actor& owner) override;
	void load(TCODZip& zip) override;
	void save(TCODZip& zip) override;
private:
	int get_next_level_xp(Actor& owner);
	void levelup_update(Actor& owner);
	void pick_item(Actor& owner);
	void drop_item(Actor& owner);
	bool is_pickable_at_position(const Actor& actor, const Actor& owner) const;
	bool try_pick_actor(std::unique_ptr<Actor> actor, Actor& owner);
	void display_inventory_items(WINDOW* inv, const Actor& owner) noexcept;
	void display_inventory(Actor& owner);
	Actor* chose_from_inventory(Actor& owner, int ascii);
	bool move_or_attack(Actor& owner, Vector2D target);
	void call_action(Actor& owner, Controls key);
};

#endif // !AIPLAYER_H
