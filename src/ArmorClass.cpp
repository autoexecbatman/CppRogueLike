#include <algorithm>
#include <variant>

#include "ArmorClass.h"
#include "ArmorMaterial.h"
#include "BuffSystem.h"
#include "Creature.h"
#include "DataManager.h"
#include "DexterityAttributes.h"
#include "EquipmentSlot.h"
#include "GameContext.h"
#include "Item.h"
#include "ItemEnhancements.h"
#include "ItemIdentification.h"
#include "MagicalItemEffects.h"
#include "Pickable.h"
#include "VariantVisitor.h"

namespace CppRL
{

namespace
{

	// A worn cloak's plus when its effect is protection, and 0 for anything
	// else. Every cloak record is made with the Gauntlets behaviour.
	//
	// Example:
	//   protection_plus(*cloakOfProtection.behavior);   // -> 1
	//   protection_plus(*cloakOfElvenkind.behavior);   // -> 0, no protection
	int protection_plus(const ItemBehavior& behavior)
	{
		return std::visit(
			VariantVisitor{
				[](const Gauntlets& cloak) -> int
				{
					return cloak.effect == MagicalEffect::PROTECTION ?
						cloak.bonus :
						0;
				},
				[]([[maybe_unused]] const auto& otherBehavior) -> int
				{ return 0; },
			},
			behavior);
	}

	// Whether a cloak of protection works over what its wearer has on: it
	// "cannot function in conjunction with any sort of magical armor, normal
	// armor not made of leather, or with a shield of any sort"
	// (DMG, PDF page 952).
	//
	// Example:
	//   can_cloak_protect(wearerInLeather);   // -> true
	//   can_cloak_protect(wearerInChainMail);   // -> false, metal
	bool can_cloak_protect(const Creature& wearer)
	{
		if (const Item* body = wearer.get_equipped_item(EquipmentSlot::BODY))
		{
			const Armor* armor =
				body->behavior ? std::get_if<Armor>(&*body->behavior) : nullptr;
			if (armor == nullptr || armor->material != ArmorMaterial::LEATHER ||
				body->get_enhancement().isMagical)
			{
				return false;
			}
		}

		const Item* held = wearer.get_equipped_item(EquipmentSlot::LEFT_HAND);
		return held == nullptr || !held->behavior ||
			!std::holds_alternative<Shield>(*held->behavior);
	}

} // namespace

ArmorClass::ArmorClass(int baseAC) :
	armorClass(baseAC),
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
	breakdown.total = breakdown.base + breakdown.dexterity +
		breakdown.equipment + breakdown.temporary;
	breakdown.changed = (breakdown.previous != breakdown.total);

	if (breakdown.changed)
	{
		set_armor_class(breakdown.total);
	}

	return breakdown;
}

int ArmorClass::without_dexterity_bonus(
	const Creature& owner,
	GameContext& ctx) const
{
	// A bonus counts downward, so only a negative adjustment is taken back out.
	return armorClass - std::min(0, calculate_dexterity_ac_bonus(owner, ctx));
}

[[nodiscard]] int ArmorClass::calculate_dexterity_ac_bonus(
	const Creature& owner,
	GameContext& ctx) const
{
	return ctx.dataManager->dexterity_for(owner.get_dexterity()).DefensiveAdj;
}

[[nodiscard]] int ArmorClass::calculate_equipment_ac_bonus(
	const Creature& owner,
	ArmorClassBreakdown& breakdown) const
{
	int totalBonus = 0;

	if (Item* equippedArmor = owner.get_equipped_item(EquipmentSlot::BODY))
	{
		int armorBonus = equippedArmor->behavior ?
			get_item_ac_bonus(*equippedArmor->behavior) :
			0;

		if (equippedArmor->get_enhancement().blessing == BlessingStatus::CURSED)
		{
			armorBonus += 1;
		}

		if (armorBonus != 0)
		{
			totalBonus += armorBonus;

			breakdown.armor = { equippedArmor->get_name(), armorBonus };
		}
	}

	if (Item* equippedShield =
			owner.get_equipped_item(EquipmentSlot::LEFT_HAND))
	{
		int shieldBonus = equippedShield->behavior ?
			get_item_ac_bonus(*equippedShield->behavior) :
			0;

		if (equippedShield->get_enhancement().blessing ==
			BlessingStatus::CURSED)
		{
			shieldBonus += 1;
		}

		if (shieldBonus != 0)
		{
			totalBonus += shieldBonus;

			breakdown.shield = { equippedShield->get_name(), shieldBonus };
		}
	}

	// AD&D 2e: best ring applies, no stacking
	int bestRingBonus = 0;
	const Item* bestRing = nullptr;

	for (
		const auto slot :
		{ EquipmentSlot::RIGHT_RING, EquipmentSlot::LEFT_RING })
	{
		if (Item* equippedRing = owner.get_equipped_item(slot))
		{
			const int ringBonus = equippedRing->behavior ?
				get_item_ac_bonus(*equippedRing->behavior) :
				0;
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

		breakdown.ring = { bestRing->get_name(), bestRingBonus };
	}

	if (Item* equippedHelm = owner.get_equipped_item(EquipmentSlot::HEAD))
	{
		const int helmBonus = equippedHelm->behavior ?
			get_item_ac_bonus(*equippedHelm->behavior) :
			0;
		if (helmBonus < 0)
		{
			totalBonus += helmBonus;

			breakdown.helm = { equippedHelm->get_name(), helmBonus };
		}
	}

	// A cloak of protection stacks with a ring of protection, which is
	// "cumulative with all other magical items of protection" (DMG page 908).
	if (Item* equippedCloak = owner.get_equipped_item(EquipmentSlot::CLOAK))
	{
		const int cloakBonus = equippedCloak->behavior ?
			-protection_plus(*equippedCloak->behavior) :
			0;
		if (cloakBonus < 0 && can_cloak_protect(owner))
		{
			totalBonus += cloakBonus;

			breakdown.cloak = { equippedCloak->get_name(), cloakBonus };
		}
	}

	return totalBonus;
}

} // namespace CppRL
