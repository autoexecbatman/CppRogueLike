#pragma once

#include "../Actor/Pickable.h"
#include "../Actor/Actor.h"

class CorpseFood : public Pickable {
public:
    CorpseFood(int nutrition_value);

    bool use(Item& owner, Creature& wearer) override;
    void load(const json& j) override;
    void save(json& j) override;
    PickableType get_type() const override { return PickableType::CORPSE_FOOD; }

private:
    int nutrition_value;  // How much hunger this food reduces
};