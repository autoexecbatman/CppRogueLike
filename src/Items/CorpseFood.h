#pragma once

#include "../Persistent/Persistent.h"
#include "../Actor/Pickable.h"

class Creature;
struct GameContext;

class CorpseFood : public Pickable
{
public:
    CorpseFood(int nutrition_value);
    ~CorpseFood() override = default;
    CorpseFood(const CorpseFood&) = delete;
    CorpseFood& operator=(const CorpseFood&) = delete;
    CorpseFood(CorpseFood&&) noexcept = default;
    CorpseFood& operator=(CorpseFood&&) noexcept = default;

    bool use(Item& owner, Creature& wearer, GameContext& ctx) override;
    void load(const json& j) override;
    void save(json& j) override;
    PickableType get_type() const override { return PickableType::CORPSE_FOOD; }

private:
    int nutrition_value;  // How much hunger this food reduces
};
