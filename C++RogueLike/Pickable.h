#pragma once
class Pickable
{
public:
	virtual ~Pickable() {};

	bool pick(Actor* owner, Actor* wearer);
	virtual bool use(Actor* owner, Actor* wearer);
};

class Healer : public Pickable
{
public:
	float amount; // how many hp

	Healer(float amount);
	bool use(Actor* owner, Actor* wearer);
};

class LightningBolt : public Pickable
{
public:
	float range, damage;
	LightningBolt(float range, float damage);
	bool use(Actor* owner, Actor* wearer);
};