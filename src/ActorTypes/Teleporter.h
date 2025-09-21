#pragma once

#include <libtcod.h>

#include "../Actor/Actor.h"
#include "../Actor/Pickable.h"

//==TELEPORTER==
//==
class Teleporter : public Pickable
{
public:
	Teleporter();

	bool use(Item& owner, Creature& wearer) override;

	void load(const json& j) override;
	void save(json& j) override;
	PickableType get_type() const override;

private:
	Vector2D find_valid_teleport_location();
};
//====
