#pragma once

//====

class Destructible
{
public:
	float maxHp = 0; // maximum health points
	float hp = 0; // current health points
	float defense = 0; // hit points deflected
	const char* corpseName = "init_name"; // the actor's name once it is dead/destroyed
	
	Destructible(
		float maxHp,
		float defense,
		const char* corpseName
	);

	bool isDead() { return hp <= 0; } // is the actor dead?
	//====
	//handles damage, owner attacked, returns (dam - def)
	float takeDamage(Actor* owner, float damage);
	//====
	//handles death, owner killed
	virtual void die(Actor* owner);
	// The function returns the amount of health point actually restored.
	float heal(float amount);
};

//====

class MonsterDestructible : public Destructible
{
public:
	MonsterDestructible(
		float maxHp,
		float defense,
		const char* corpseName
	);
	//====
	//handles death, owner killed
	void die(Actor* owner);
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
};

//====