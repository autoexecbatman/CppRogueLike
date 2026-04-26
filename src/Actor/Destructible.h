#pragma once

#include "../Combat/DamageInfo.h"
#include "../Persistent/Persistent.h"

class Creature;
struct GameContext;

class Destructible : public Persistent
{
public:
    explicit Destructible() = default;
    virtual ~Destructible() override = default;
    Destructible(const Destructible&) = delete;
    Destructible(Destructible&&) = delete;
    Destructible& operator=(const Destructible&) = delete;
    Destructible& operator=(Destructible&&) = delete;

    // Action method delegating to Creature's health pool
    int take_damage(Creature& owner, int damage, GameContext& ctx, DamageType damageType = DamageType::PHYSICAL);

    void load(const json& j) override;
    void save(json& j) override;

    [[nodiscard]] static std::unique_ptr<Destructible> create(const json& j);
};
