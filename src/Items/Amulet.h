#pragma once

#include "../Actor/Pickable.h"
#include "../Actor/Actor.h"

// Amulet class - the item needed to win the game
class Amulet : public Pickable {
public:
    Amulet();

    bool use(Item& owner, Creature& wearer, GameContext& ctx) override;
    void load(const json& j) override;
    void save(json& j) override;
    PickableType get_type() const override { return PickableType::AMULET; }
};