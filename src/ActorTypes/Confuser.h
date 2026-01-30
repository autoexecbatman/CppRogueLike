#pragma once

#include "../Actor/Pickable.h"

class Creature;
class Item;
struct GameContext;

class Confuser : public Pickable
{
public:
	int nbTurns{ 0 };
	int range{ 0 };

	Confuser(int nbTurns, int range) noexcept;
	~Confuser() override = default;
	Confuser(const Confuser&) = delete;
	Confuser& operator=(const Confuser&) = delete;
	Confuser(Confuser&&) noexcept = default;
	Confuser& operator=(Confuser&&) noexcept = default;

	bool use(Item& owner, Creature& wearer, GameContext& ctx);

	void load(const json& j) override;
	void save(json& j) override;
	PickableType get_type() const override { return PickableType::CONFUSER; }
};
