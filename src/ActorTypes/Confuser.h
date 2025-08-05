#pragma once

#include "../Actor/Pickable.h"

class TCODZip;
class Creature;
class Item;

//==CONFUSER==
//==
class Confuser : public Pickable
{
public:
	int nbTurns = 0;
	int range = 0;

	Confuser(int nbTurns, int range) noexcept;

	bool use(Item& owner, Creature& wearer);

	void load(const json& j) override;
	void save(json& j) override;
	PickableType get_type() const override { return PickableType::CONFUSER; }
};
//====