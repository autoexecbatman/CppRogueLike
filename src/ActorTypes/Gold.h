#pragma once
#include "../Actor/Pickable.h"

class Gold : public Pickable
{
	public:
	int amount{ 0 };

	Gold(int amount);

	bool use(Item& owner, Creature& wearer) override;

	void save(json& j) override;
	void load(const json& j) override;
	PickableType get_type() const override { return PickableType::GOLD; }
};