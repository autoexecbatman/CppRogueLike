// Armor.h
#pragma once

#include "../Actor/Pickable.h"
#include "../Actor/Actor.h"

// Base armor class (similar to the weapon class in Pickable.h)
class Armor : public Pickable
{
public:
    int armorClass{};   // Bonus to character's AC

    // Clean armor equip/unequip logic - delegates to Equipment System
    bool use(Item& owner, Creature& wearer, GameContext& ctx) override;

    // Pure stat effects for NPCs - no inventory management
    void apply_stat_effects(Creature& creature, Item& owner, GameContext& ctx);

    // Gets the armor class bonus
    virtual int getArmorClass() const { return armorClass; }
};

// Leather Armor implementation
class LeatherArmor : public Armor
{
public:
    // Constructor
    LeatherArmor();

    void save(json& j) override;
    void load(const json& j) override;
    PickableType get_type() const override { return PickableType::LEATHER_ARMOR; }
};

// Chain Mail implementation
class ChainMail : public Armor
{
public:
    // Constructor
    ChainMail();

    void save(json& j) override;
    void load(const json& j) override;
    PickableType get_type() const override { return PickableType::CHAIN_MAIL; }
};

// Plate Mail implementation
class PlateMail : public Armor
{
public:
    // Constructor
    PlateMail();

    void save(json& j) override;
    void load(const json& j) override;
    PickableType get_type() const override { return PickableType::PLATE_MAIL; }
};