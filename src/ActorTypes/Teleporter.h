#pragma once

#include "../Actor/Actor.h"
#include "../Actor/Pickable.h"
#include "../Persistent/Persistent.h"

//==TELEPORTER==
//==
class Teleporter : public Pickable
{
public:
	Teleporter();

	bool use(Item& owner, Creature& wearer, GameContext& ctx) override;

	void load(const json& j) override;
	void save(json& j) override;
	PickableType get_type() const override;

private:
	Vector2D find_valid_teleport_location(GameContext& ctx);
};
//====
