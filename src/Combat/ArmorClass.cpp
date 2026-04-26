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

void ArmorClass::update(Creature& owner, GameContext& ctx)
{
	const int baseAC = get_base_armor_class();
	const int dexBonus = calculate_dexterity_ac_bonus(owner, ctx);
	const int equipBonus = calculate_equipment_ac_bonus(owner, ctx);
	const int tempBonus = ctx.buffSystem->calculate_ac_bonus(owner);
	const int calculatedAC = baseAC + dexBonus + equipBonus + tempBonus;

	if (get_armor_class() != calculatedAC)
	{
		const int oldAC = get_armor_class();
		set_armor_class(calculatedAC);

		if (&owner == ctx.player)
		{
			ctx.messageSystem->log(std::format(
				"Armor Class updated: {} -> {} (Base: {}, Dex: {:+}, Equipment: {:+}, Temp: {:+})",
				oldAC,
				calculatedAC,
				baseAC,
				dexBonus,
				equipBonus,
				tempBonus));
		}
	}
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

	if (&owner == ctx.player && defensiveAdj != 0)
	{
		ctx.messageSystem->log(std::format(
			"Dexterity Defensive Adjustment: {:+} (Dex: {})",
			defensiveAdj,
			dexterity));
	}

	return defensiveAdj;
}

[[nodiscard]] int ArmorClass::calculate_equipment_ac_bonus(const Creature& owner, GameContext& ctx) const
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

			if (&owner == ctx.player)
			{
				ctx.messageSystem->log(std::format(
					"Armor bonus: {:+} from {}",
					armorBonus,
					equippedArmor->actorData.name));
			}
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

			if (&owner == ctx.player)
			{
				ctx.messageSystem->log(std::format(
					"Shield bonus: {:+} from {}",
					shieldBonus,
					equippedShield->actorData.name));
			}
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

		if (&owner == ctx.player)
		{
			ctx.messageSystem->log(std::format(
				"Ring bonus: {:+} from {}",
				bestRingBonus,
				bestRing->actorData.name));
		}
	}

	if (Item* equippedHelm = owner.get_equipped_item(EquipmentSlot::HEAD))
	{
		const int helmBonus = equippedHelm->behavior ? get_item_ac_bonus(*equippedHelm->behavior) : 0;
		if (helmBonus < 0)
		{
			totalBonus += helmBonus;

			if (&owner == ctx.player)
			{
				ctx.messageSystem->log(std::format(
					"Helm bonus: {:+} from {}",
					helmBonus,
					equippedHelm->actorData.name));
			}
		}
	}

	return totalBonus;
}
