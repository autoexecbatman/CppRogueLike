#pragma once

//====

class Destructible : public Persistent
{
public:
	float maxHp = 0; // maximum health points
	float hp = 0; // current health points
	float defense = 0; // hit points deflected
	const char* corpseName = "corpseName"; // the actor's name once it is dead/destroyed
	int xp; // for awarding experience points
	
	Destructible(
		float maxHp,
		float defense,
		const char* corpseName,
		int xp
	);

	virtual ~Destructible();

	// returns true if hp is below or equal to 0
	bool isDead() { return hp <= 0; } // is the actor dead?
	//====
	//handles damage, owner attacked, returns (dam - def)
	float takeDamage(Actor* owner, float damage);
	//====
	//handles death, owner killed
	virtual void die(Actor* owner);
	// The function returns the amount of health point actually restored.
	float heal(float amount);

	void load(TCODZip& zip);
	void save(TCODZip& zip);
	static Destructible* create(TCODZip& zip);
protected:
	enum class DestructibleType : int
	{
		MONSTER, PLAYER
	};

};

//====

class MonsterDestructible : public Destructible
{
public:
	MonsterDestructible(
		float maxHp,
		float defense,
		const char* corpseName,
		int xp
	);
	//====
	//handles death, owner killed
	void die(Actor* owner);
	void save(TCODZip& zip);
};

//====

class PlayerDestructible : public Destructible
{
public:
	PlayerDestructible(
		float maxHp,
		float defense,
		const char* corpseName
	);
	//====
	//handles death, owner killed	
	void die(Actor* owner);
	void save(TCODZip& zip);
};

//====