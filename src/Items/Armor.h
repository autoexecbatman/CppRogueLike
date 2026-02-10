#pragma once

#include "../Actor/Pickable.h"

class Creature;
class Item;
struct GameContext;

class Armor : public Pickable
{
public:
    explicit Armor(int acBonus);

    bool use(Item& owner, Creature& wearer, GameContext& ctx) override;
    void apply_stat_effects(Creature& creature, Item& owner, GameContext& ctx);

    int get_ac_bonus() const noexcept override { return armorClass; }

    void save(json& j) override;
    void load(const json& j) override;
    PickableType get_type() const override { return PickableType::ARMOR; }

private:
    int armorClass{};
};
