#pragma once

#include <libtcod.h>

#include "../Actor/Actor.h"
#include "../Actor/Pickable.h"

//==HEALER==
//==
class Healer : public Pickable
{
public:
	int amountToHeal{ 0 }; // how many hp

	Healer(int amountToHeal);

	bool use(Item& owner, Creature& wearer, GameContext& ctx) override;

	void load(const json& j) override;
	void save(json& j) override;
	PickableType get_type() const override;
};
//====