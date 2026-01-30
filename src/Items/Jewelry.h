#pragma once

#include "../Actor/Pickable.h"
#include "MagicalItemEffects.h"

class Creature;
class Item;
struct GameContext;

// Base class for magical equipment (helms, rings, etc.)
class MagicalEquipment : public Pickable
{
public:
    MagicalEffect effect{MagicalEffect::NONE};

    MagicalEquipment() = default;
    explicit MagicalEquipment(MagicalEffect magical_effect) : effect(magical_effect) {}

    // Equip/unequip logic - delegates to Equipment System
    bool use(Item& owner, Creature& wearer, GameContext& ctx) override;

    // Apply magical effect to creature
    void apply_effect(Creature& creature, Item& owner, GameContext& ctx);

    // Get the appropriate equipment slot
    virtual EquipmentSlot get_equipment_slot() const = 0;

protected:
    // Template method for DRY save/load
    void save_magical_effect(json& j, PickableType type) const;
    void load_magical_effect(const json& j);
};

// Authentic AD&D 2e Magical Helms - provide special abilities, NOT AC bonuses
class MagicalHelm : public MagicalEquipment
{
public:
    MagicalHelm() = default;
    explicit MagicalHelm(MagicalEffect helm_effect) : MagicalEquipment(helm_effect) {}

    void save(json& j) override;
    void load(const json& j) override;
    PickableType get_type() const override { return PickableType::HELMET; }
    EquipmentSlot get_equipment_slot() const override;
};

// Authentic AD&D 2e Magical Rings
class MagicalRing : public MagicalEquipment
{
public:
    MagicalRing() = default;
    explicit MagicalRing(MagicalEffect ring_effect) : MagicalEquipment(ring_effect) {}

    void save(json& j) override;
    void load(const json& j) override;
    PickableType get_type() const override { return PickableType::RING; }
    EquipmentSlot get_equipment_slot() const override;
};

// Base class for equipment with stat bonuses (gauntlets, girdles)
class StatBoostEquipment : public Pickable
{
public:
    int str_bonus{0};
    int dex_bonus{0};
    int con_bonus{0};
    int int_bonus{0};
    int wis_bonus{0};
    int cha_bonus{0};

    // If true, non-zero bonuses SET stats to that value instead of adding
    bool is_set_mode{false};

    // Original stats before SET operation (stored on equip, restored on unequip)
    struct OriginalStats {
        int str{0};
        int dex{0};
        int con{0};
        int intel{0};
        int wis{0};
        int cha{0};
    } original_stats;

    // Equip/unequip logic
    bool use(Item& owner, Creature& wearer, GameContext& ctx) override;

    // Apply stat bonuses
    void apply_stat_bonuses(Creature& creature, Item& owner, GameContext& ctx);

    // Get the appropriate equipment slot
    virtual EquipmentSlot get_equipment_slot() const = 0;

protected:
    // Template method for DRY save/load
    void save_stat_bonuses(json& j, PickableType type) const;
    void load_stat_bonuses(const json& j);
};

// Legacy support - keep old Helmet/Ring classes for compatibility during transition
class Helmet : public MagicalHelm
{
public:
    Helmet() : MagicalHelm(MagicalEffect::NONE) {}
};

class Ring : public MagicalRing
{
public:
    Ring() : MagicalRing(MagicalEffect::NONE) {}
};

// Amulet placeholder
class JewelryAmulet : public StatBoostEquipment
{
public:
    void save(json& j) override;
    void load(const json& j) override;
    PickableType get_type() const override { return PickableType::AMULET; }
    EquipmentSlot get_equipment_slot() const override;
};

// Authentic AD&D 2e Gauntlets - provide stat bonuses
class Gauntlets : public StatBoostEquipment
{
public:
    void save(json& j) override;
    void load(const json& j) override;
    PickableType get_type() const override { return PickableType::GAUNTLETS; }
    EquipmentSlot get_equipment_slot() const override;
};

// Authentic AD&D 2e Girdle - provide stat bonuses
class Girdle : public StatBoostEquipment
{
public:
    void save(json& j) override;
    void load(const json& j) override;
    PickableType get_type() const override { return PickableType::GIRDLE; }
    EquipmentSlot get_equipment_slot() const override;
};
