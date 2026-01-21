#pragma once

#include "../Actor/Pickable.h"
#include "../Persistent/Persistent.h"

class Creature;
class Item;
struct GameContext;
struct Vector2D;

class LightningBolt : public Pickable
{
public:
	int maxRange{};
	int damage{};

	LightningBolt(int range, int damage) noexcept;

	bool use(Item& owner, Creature& wearer, GameContext& ctx) override;

	// Lightning animation between two points
	void animate_lightning(Vector2D from, Vector2D to, GameContext& ctx);

	void load(const json& j) override;
	void save(json& j) override;
	PickableType get_type() const override { return PickableType::LIGHTNING_BOLT; }
};
