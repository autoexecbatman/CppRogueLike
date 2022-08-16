#pragma once

//====

class Destructible
{
public:
	float maxHp = 0;
	float hp = 0;
	float defense = 0;
	const char* corpseName = 0;
	
	Destructible(float maxHp, float defense, const char* corpseName);
	inline bool isDead() { return hp <= 0; }
	float takeDamage(Actor* owner, float damage);
	virtual void die(Actor* owner);
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
	void die(Actor* owner);
};

//====