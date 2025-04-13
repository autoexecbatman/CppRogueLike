// Armor.h
#pragma once

#include "Actor/Pickable.h"
#include "Actor/Actor.h"

// Base armor class (similar to the weapon class in Pickable.h)
class Armor : public Pickable
{
public:
    int armorClass;   // Bonus to character's AC

    // Common armor equip/unequip logic
    bool use(Item& owner, Creature& wearer) override;

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