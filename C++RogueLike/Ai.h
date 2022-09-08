#pragma once

//====

class Ai
{
public:
	virtual ~Ai() {};
	virtual void update(Actor* owner) = 0;
};

//====

class MonsterAi : public Ai
{
public:
	MonsterAi();
	void update(Actor* owner);
	
protected:
	int moveCount = 0;

	void moveOrAttack(Actor* owner, int targetx, int targety);
};

//====

class PlayerAi : public Ai
{
public:
	void update(Actor* owner);
	void handleActionKey(Actor* owner, int ascii);
	
protected:
	Actor* choseFromInventory(Actor* owner);
	bool moveOrAttack(Actor* owner, int targetx, int targety);
};

//====