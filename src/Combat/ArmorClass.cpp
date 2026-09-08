#include <cassert>
#include <format>

#include "../Actor/Creature.h"
#include "../Actor/EquipmentSlot.h"
#include "../Actor/Pickable.h"
#include "../Attributes/DexterityAttributes.h"
#include "../Colors/Colors.h"
#include "../Core/GameContext.h"
#include "../Systems/BuffSystem.h"
#include "../Systems/DataManager.h"
#include "../Systems/MessageSystem.h"
#include "ArmorClass.h"

ArmorClass::ArmorClass(int baseAC)
	: armorClass(baseAC),
	  baseArmorClass(baseAC)
{
}

// Recomputes the armour class and reports how it was reached.
//
// AD&D 2e counts downward: every bonus below is negative, and a lower total is
// better armour. The returned breakdown carries each contribution so a caller
// that wants to show the arithmetic can, without this function knowing who is
// watching.
//
// Example:
//   const ArmorClassBreakdown ac = armorClass->update(owner, ctx);
//   ac.changed;            // -> true when the total moved
//   ac.previous, ac.total; // -> 10, 5
//   ac.armor.name;         // -> "plate mail", or empty if the slot is bare
ArmorClassBreakdown ArmorClass::update(Creature& owner, GameContext& ctx)
{
	ArmorClassBreakdown breakdown{};
	breakdown.previous = get_armor_class();
	breakdown.base = get_base_armor_class();
	breakdown.dexterity = calculate_dexterity_ac_bonus(owner, ctx);
	breakdown.equipment = calculate_equipment_ac_bonus(owner, breakdown);
	breakdown.temporary = ctx.buffSystem->calculate_ac_bonus(owner);
	breakdown.total = breakdown.base + breakdown.dexterity + breakdown.equipment + breakdown.temporary;
	breakdown.changed = (breakdown.previous != breakdown.total);

	if (breakdown.changed)
	{
		set_armor_class(breakdown.total);
	}

	return breakdown;
}

[[nodiscard]] int ArmorClass::calculate_dexterity_ac_bonus(const Creature& owner, GameContext& ctx) const
{
	const auto& dexAttributes = ctx.dataManager->get_dexterity_attributes();
	const int dexterity = owner.get_dexterity();

	if (dexterity <= 0 || dexterity > static_cast<int>(dexAttributes.size()))
	{
		return 0;
	}

	const int defensiveAdj = dexAttributes[dexterity - 1].DefensiveAdj;

	return defensiveAdj;
}

[[nodiscard]] int ArmorClass::calculate_equipment_ac_bonus(const Creature& owner, ArmorClassBreakdown& breakdown) const
{
	int totalBonus = 0;

	if (Item* equippedArmor = owner.get_equipped_item(EquipmentSlot::BODY))
	{
		int armorBonus = equippedArmor->behavior ? get_item_ac_bonus(*equippedArmor->behavior) : 0;

		if (equippedArmor->get_enhancement().blessing == BlessingStatus::CURSED)
		{
			armorBonus += 1;
		}

		if (armorBonus != 0)
		{
			totalBonus += armorBonus;

			breakdown.armor = { equippedArmor->actorData.name, armorBonus };
		}
	}

	if (Item* equippedShield = owner.get_equipped_item(EquipmentSlot::LEFT_HAND))
	{
		int shieldBonus = equippedShield->behavior ? get_item_ac_bonus(*equippedShield->behavior) : 0;

		if (equippedShield->get_enhancement().blessing == BlessingStatus::CURSED)
		{
			shieldBonus += 1;
		}

		if (shieldBonus != 0)
		{
			totalBonus += shieldBonus;

			breakdown.shield = { equippedShield->actorData.name, shieldBonus };
		}
	}

	// AD&D 2e: best ring applies, no stacking
	int bestRingBonus = 0;
	const Item* bestRing = nullptr;

	for (const auto slot : { EquipmentSlot::RIGHT_RING, EquipmentSlot::LEFT_RING })
	{
		if (Item* equippedRing = owner.get_equipped_item(slot))
		{
			const int ringBonus = equippedRing->behavior ? get_item_ac_bonus(*equippedRing->behavior) : 0;
			if (ringBonus < bestRingBonus)
			{
				bestRingBonus = ringBonus;
				bestRing = equippedRing;
			}
		}
	}

	if (bestRingBonus < 0 && bestRing)
	{
		totalBonus += bestRingBonus;

		breakdown.ring = { bestRing->actorData.name, bestRingBonus };
	}

	if (Item* equippedHelm = owner.get_equipped_item(EquipmentSlot::HEAD))
	{
		const int helmBonus = equippedHelm->behavior ? get_item_ac_bonus(*equippedHelm->behavior) : 0;
		if (helmBonus < 0)
		{
			totalBonus += helmBonus;

			breakdown.helm = { equippedHelm->actorData.name, helmBonus };
		}
	}

	return totalBonus;
}
