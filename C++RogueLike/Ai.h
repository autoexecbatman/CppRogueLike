// file: Ai.h
#ifndef AI_H
#define AI_H

#include <curses.h>

//==AI==
class Ai : public Persistent
{
public:
	Ai() = default;
	virtual ~Ai() {};
	virtual void update(Actor& owner) = 0;
	/*static Ai* create(TCODZip& zip);*/

	static std::shared_ptr<Ai> create(TCODZip& zip);
	// Defaulted copy constructor and copy assignment operator
	Ai(const Ai&) = default;
	Ai& operator=(const Ai&) = default;

	// Defaulted move constructor and move assignment operator
	Ai(Ai&&) noexcept = default;
	Ai& operator=(Ai&&) noexcept = default;

protected:
	enum class AiType 
	{
		MONSTER, CONFUSED_MONSTER, PLAYER
	};
};

//==MONSTER_AI==
class MonsterAi : public Ai
{
public:
	MonsterAi();
	void update(Actor& owner);

	void load(TCODZip& zip);
	void save(TCODZip& zip);
	
protected:
	int moveCount = 0;

	void moveOrAttack(Actor& owner, int targetx, int targety);
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
