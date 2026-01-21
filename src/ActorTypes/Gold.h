#pragma once

#include "../Persistent/Persistent.h"
#include "../Actor/Pickable.h"

class Item;
class Creature;
struct GameContext;

class Gold : public Pickable
{
	public:
	int amount{ 0 };

	Gold(int amount);

	bool use(Item& owner, Creature& wearer, GameContext& ctx) override;

	void save(json& j) override;
	void load(const json& j) override;
	PickableType get_type() const override { return PickableType::GOLD; }
};