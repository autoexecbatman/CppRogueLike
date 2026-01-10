#ifndef LIGHTNING_BOLT_H
#define LIGHTNING_BOLT_H

#pragma once

#include <libtcod.h>

#include "../Actor/Actor.h"
#include "../Actor/Pickable.h"

//==LIGHTNING_BOLT==
//==
class LightningBolt : public Pickable
{
public:
	int maxRange = 0;
	int damage = 0;

	LightningBolt(int range, int damage) noexcept;

	bool use(Item& owner, Creature& wearer, GameContext& ctx) override;

	// Lightning animation between two points
	void animate_lightning(Vector2D from, Vector2D to, GameContext& ctx);

	void load(const json& j) override;
	void save(json& j) override;
	PickableType get_type() const override { return PickableType::LIGHTNING_BOLT; }
};
//====
#endif // !LIGHTNING_BOLT_H
