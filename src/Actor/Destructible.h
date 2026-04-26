#pragma once

#include <algorithm>
#include <memory>
#include <string>

#include "../Combat/ArmorClass.h"
#include "../Combat/ConstitutionTracker.h"
#include "../Combat/DamageInfo.h"
#include "../Combat/DeathHandler.h"
#include "../Combat/HealthPool.h"
#include "../Persistent/Persistent.h"

class Creature;
struct GameContext;

class Destructible : public Persistent
{
private:
    std::unique_ptr<DeathHandler> deathHandler{};
    std::unique_ptr<ArmorClass> armorClass{};
    std::unique_ptr<ConstitutionTracker> constitutionTracker{};
    std::unique_ptr<HealthPool> healthPool{};

    void handle_stat_drain_death(Creature& owner, GameContext& ctx);
    void log_constitution_change(const Creature& owner, GameContext& ctx, int oldCon, int newCon, int hpChange) const;

public:
    Destructible(
        int hpMax,
        int armorClass,
        std::unique_ptr<DeathHandler> handler);
    virtual ~Destructible() override = default;
    Destructible(const Destructible&) = delete;
    Destructible(Destructible&&) = delete;
    Destructible& operator=(const Destructible&) = delete;
    Destructible& operator=(Destructible&&) = delete;

    void update_constitution_bonus(Creature& owner, GameContext& ctx);

    // Query methods
    [[nodiscard]] bool is_dead() const noexcept { return healthPool->is_dead(); }
    [[nodiscard]] int get_hp() const noexcept { return healthPool->get_hp(); }
    [[nodiscard]] int get_max_hp() const noexcept { return healthPool->get_max_hp(); }
    [[nodiscard]] int get_armor_class() const noexcept { return armorClass->get_armor_class(); }
    [[nodiscard]] int get_base_armor_class() const noexcept { return armorClass->get_base_armor_class(); }
    [[nodiscard]] int get_last_constitution() const noexcept { return constitutionTracker->get_last_constitution(); }
    [[nodiscard]] int get_hp_base() const noexcept { return healthPool->get_hp_base(); }

    [[nodiscard]] int get_temp_hp() const noexcept { return healthPool->get_temp_hp(); }
    void set_temp_hp(int value) noexcept { healthPool->set_temp_hp(value); }
    void add_temp_hp(int amount) noexcept { healthPool->add_temp_hp(amount); }

    [[nodiscard]] int get_effective_hp() const noexcept { return healthPool->get_effective_hp(); }

    // Modifier methods
    void set_hp(int value) noexcept { healthPool->set_hp(value); }
    void set_max_hp(int value) noexcept { healthPool->set_max_hp(value); }
    void set_hp_base(int value) noexcept { healthPool->set_hp_base(value); }
    void set_armor_class(int value) noexcept { armorClass->set_armor_class(value); }
    void set_base_armor_class(int value) noexcept { armorClass->set_base_armor_class(value); }
    void set_last_constitution(int value) noexcept { constitutionTracker->set_last_constitution(value); }

    // Action methods
    int take_damage(Creature& owner, int damage, GameContext& ctx, DamageType damageType = DamageType::PHYSICAL)
    {
        const int actual = healthPool->take_damage(owner, damage, ctx, damageType);
        if (is_dead())
        {
            die(owner, ctx);
        }
        return actual;
    }
    void die(Creature& owner, GameContext& ctx);
    [[nodiscard]] int heal(int hpToHeal) { return healthPool->heal(hpToHeal); }
    void update_armor_class(Creature& owner, GameContext& ctx);
    [[nodiscard]] const ArmorClass* get_armor_class_system() const noexcept { return armorClass.get(); }

    void load(const json& j) override;
    void save(json& j) override;

    [[nodiscard]] static std::unique_ptr<Destructible> create(const json& j);
};
