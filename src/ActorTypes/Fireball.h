#pragma once

#include <libtcod.h>

#include "../Actor/Actor.h"
#include "LightningBolt.h"

//==FIREBALL==
//==
class Fireball : public LightningBolt
{
public:
	Fireball(int range, int damage);

	bool use(Item& owner, Creature& wearer) override;

	void animation(Vector2D position, int maxRange);

	void load(const json& j);
	void save(json& j);
};
//====