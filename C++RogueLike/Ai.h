// file: Ai.h
#ifndef AI_H
#define AI_H

#include <curses.h>

#include "Persistent.h"

class Actor; // for no circular dependency with Actor.h

//==AI==
class Ai : public Persistent
{
public:
	// Defaulted constructor and destructor
	Ai() = default;
	virtual ~Ai() {};

	// Defaulted copy constructor and copy assignment operator
	Ai(const Ai&) = default;
	Ai& operator=(const Ai&) = default;

	// Defaulted move constructor and move assignment operator
	Ai(Ai&&) noexcept = default;
	Ai& operator=(Ai&&) noexcept = default;

	virtual void update(Actor& owner) = 0;

	static std::shared_ptr<Ai> create(TCODZip& zip);

protected:
	enum class AiType
	{
		MONSTER, CONFUSED_MONSTER, PLAYER
	};
};

//==PLAYER_AI==
class PlayerAi : public Ai
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

//====
class ConfusedMonsterAi : public Ai
{
public:
	ConfusedMonsterAi(int nbTurns, std::shared_ptr<Ai> oldAi);
	void update(Actor& owner);
	void load(TCODZip& zip);
	void save(TCODZip& zip);
	
protected:
	int nbTurns;
	std::shared_ptr<Ai> oldAi;
};

#endif // !AI_H
// file: Ai.h
