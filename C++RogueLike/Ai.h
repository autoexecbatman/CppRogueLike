#pragma once

//==AI==
class Ai : public Persistent
{
public:
	virtual ~Ai() {};
	virtual void update(Actor* owner) = 0;
	static Ai* create(TCODZip& zip);
protected:
	enum AiType {
		MONSTER, CONFUSED_MONSTER, PLAYER
	};
};

//==MONSTER_AI==
class MonsterAi : public Ai
{
public:
	MonsterAi();
	void update(Actor* owner);

	void load(TCODZip& zip);
	void save(TCODZip& zip);
	
protected:
	int moveCount = 0;

	void moveOrAttack(Actor* owner, int targetx, int targety);
};

//==PLAYER_AI==
class PlayerAi : public Ai
{
public:
	void update(Actor* owner);
	void load(TCODZip& zip);
	void save(TCODZip& zip);
	
protected:
	void handleActionKey(Actor* owner, int ascii);
	Actor* choseFromInventory(Actor* owner, int ascii);
	bool moveOrAttack(Actor* owner, int targetx, int targety);
};

//====