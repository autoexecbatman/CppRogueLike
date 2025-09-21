#pragma once

#include "../Actor/Pickable.h"

class Mace : public Pickable
{
public:
    Mace();
    
    bool use(Item& owner, Creature& wearer) override;
    void load(const json& j) override;
    void save(json& j) override;
    PickableType get_type() const override;
};
