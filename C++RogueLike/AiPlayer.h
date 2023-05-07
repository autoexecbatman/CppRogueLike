// file: Ai.h
#ifndef AIPLAYER_H
#define AIPLAYER_H

#include <curses.h>
#include <libtcod.h>

#include "Actor.h"
#include "Ai.h"


//==PLAYER_AI==
class AiPlayer : public Ai
{
public:
	int xpLevel{ 1 };

	int getNextLevelXp();
	bool levelUpUpdate(Actor& owner);
	void update(Actor& owner) override;
	void load(TCODZip& zip); // should be marked as override ? No, because it's not a virtual function
	void save(TCODZip& zip);

	void pick_item(Actor& owner);
	void display_inventory(Actor& owner);

protected:
	/*void handleActionKey(Actor* owner, int ascii);*/
	std::shared_ptr<Actor> choseFromInventory(Actor& owner, int ascii);
	bool moveOrAttack(Actor& owner, int targetx, int targety);
private:
	bool is_pickable_at_position(const Actor& actor, const Actor& owner) const;
	bool try_pick_actor(Actor& actor, Actor& owner);
	void displayInventoryItems(WINDOW* inv, Actor& owner);

};

#endif // !AIPLAYER_H
