#pragma once

#include <algorithm>
#include <memory>
#include <string>

#include "../Combat/ArmorClass.h"
#include "../Combat/ConstitutionTracker.h"
#include "../Combat/DamageInfo.h"
#include "../Combat/DeathHandler.h"
#include "../Persistent/Persistent.h"

class Creature;
struct GameContext;

class Destructible : public Persistent
{
private:
    int hpBase{};
    int hpMax{};
    int hp{};
    int dr{};
    std::string corpseName{};
    int xp{};
    int thaco{};
    int tempHp{};
    std::unique_ptr<DeathHandler> deathHandler{};
    std::unique_ptr<ArmorClass> armorClass{};
    std::unique_ptr<ConstitutionTracker> constitutionTracker{};

    void handle_stat_drain_death(Creature& owner, GameContext& ctx);
    void log_constitution_change(const Creature& owner, GameContext& ctx, int oldCon, int newCon, int hpChange) const;

public:
    Destructible(
        int hpMax,
        int dr,
        std::string_view corpseName,
        int xp,
        int thaco,
        int armorClass,
        std::unique_ptr<DeathHandler> handler);
    virtual ~Destructible() override = default;
    Destructible(const Destructible&) = delete;
    Destructible(Destructible&&) = delete;
    Destructible& operator=(const Destructible&) = delete;
    Destructible& operator=(Destructible&&) = delete;

    void update_constitution_bonus(Creature& owner, GameContext& ctx);

    // Query methods
    [[nodiscard]] bool is_dead() const noexcept { return hp <= 0; }
    [[nodiscard]] int get_hp() const noexcept { return hp; }
    [[nodiscard]] int get_max_hp() const noexcept { return hpMax; }
    [[nodiscard]] int get_armor_class() const noexcept { return armorClass->get_armor_class(); }
    [[nodiscard]] int get_base_armor_class() const noexcept { return armorClass->get_base_armor_class(); }
    [[nodiscard]] int get_thaco() const noexcept { return thaco; }
    [[nodiscard]] int get_dr() const noexcept { return dr; }
    [[nodiscard]] const std::string& get_corpse_name() const noexcept { return corpseName; }
    [[nodiscard]] int get_xp() const noexcept { return xp; }
    [[nodiscard]] int get_last_constitution() const noexcept { return constitutionTracker->get_last_constitution(); }
    [[nodiscard]] int get_hp_base() const noexcept { return hpBase; }

    [[nodiscard]] int get_temp_hp() const noexcept { return tempHp; }
    void set_temp_hp(int value) noexcept { tempHp = std::max(0, value); }
    void add_temp_hp(int amount) noexcept { tempHp += amount; }

    [[nodiscard]] int get_effective_hp() const noexcept { return hp + tempHp; }

    // Modifier methods
    void set_hp(int value) noexcept { hp = value; }
    void set_max_hp(int value) noexcept { hpMax = value; }
    void set_hp_base(int value) noexcept { hpBase = value; }
    void set_armor_class(int value) noexcept { armorClass->set_armor_class(value); }
    void set_base_armor_class(int value) noexcept { armorClass->set_base_armor_class(value); }
    void set_thaco(int value) noexcept { thaco = value; }
    void set_dr(int value) noexcept { dr = value; }
    void set_corpse_name(std::string_view name) { corpseName = name; }
    void set_xp(int value) noexcept { xp = value; }
    void set_last_constitution(int value) noexcept { constitutionTracker->set_last_constitution(value); }
    void add_xp(int amount) noexcept { xp += amount; }

    // Action methods
    int take_damage(Creature& owner, int damage, GameContext& ctx, DamageType damageType = DamageType::PHYSICAL);
    void die(Creature& owner, GameContext& ctx);
    [[nodiscard]] int heal(int hpToHeal);
    void update_armor_class(Creature& owner, GameContext& ctx);
    [[nodiscard]] const ArmorClass* get_armor_class_system() const noexcept { return armorClass.get(); }

    void load(const json& j) override;
    void save(json& j) override;

    [[nodiscard]] static std::unique_ptr<Destructible> create(const json& j);
};
