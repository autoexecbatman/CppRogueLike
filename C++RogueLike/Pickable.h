#pragma once
class Pickable : public Persistent
{
public:
	virtual ~Pickable() {};

	bool pick(Actor* owner, Actor* wearer);
	virtual bool use(Actor* owner, Actor* wearer);
	static Pickable* create(TCODZip& zip);
protected:
	enum class PickableType : int
	{
		HEALER, LIGHTNING_BOLT, CONFUSER, FIREBALL
	};
};

class Healer : public Pickable
{
public:
	int amountToHeal; // how many hp

	Healer(int amountToHeal);

	bool use(Actor* owner, Actor* wearer);

	void load(TCODZip& zip);
	void save(TCODZip& zip);
};

class LightningBolt : public Pickable
{
public:
	float maxRange = 0;
	float damage = 0;
	
	LightningBolt(float range, float damage);

	bool use(Actor* owner, Actor* wearer);

	void load(TCODZip& zip);
	void save(TCODZip& zip);
};

class Fireball : public LightningBolt
{
public:
	Fireball(float range, float damage);

	bool use(Actor* owner, Actor* wearer);

	void load(TCODZip& zip);
	void save(TCODZip& zip);
};