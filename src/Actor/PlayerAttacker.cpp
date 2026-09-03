#include <string>

#include "../Actor/Creature.h"
#include "../Actor/Item.h"
#include "../ActorTypes/Player.h"
#include "../Colors/Colors.h"
#include "../Combat/DamageInfo.h"
#include "../Combat/WeaponDamageRegistry.h"
#include "../Core/GameContext.h"
#include "../Items/ItemIdentification.h"
#include "../Systems/ItemEnhancements/ItemEnhancements.h"
#include "../Systems/MessageSystem.h"
#include "PlayerAttacker.h"

PlayerAttacker::PlayerAttacker(Player& owner)
	: Attacker(DamageInfo{}), owner(owner) {}

DamageInfo PlayerAttacker::compute_weapon_damage(EquipmentSlot slot) const
{
	Item* weapon = owner.get_equipped_item(slot);
	if (weapon && weapon->is_weapon())
	{
		const ItemEnhancement* enhancement = weapon->is_enhanced() ? &weapon->get_enhancement() : nullptr;
		return WeaponDamageRegistry::get_enhanced_damage_info(weapon->itemKey, enhancement);
	}
	return WeaponDamageRegistry::get_unarmed_damage_info();
}

void PlayerAttacker::attack(Creature& target, GameContext& ctx)
{
	// Every to-hit adjustment the equipped weapon confers, resolved at the point of
	// attack so there is one source of truth. AD&D 2e PHB p.88: a cursed weapon is
	// -2 to attack rolls; an enhanced weapon grants its toHitBonus.
	auto weapon_hit_modifier = [this](EquipmentSlot slot) -> int
	{
		Item* weapon = owner.get_equipped_item(slot);
		if (!weapon)
		{
			return 0;
		}

		const ItemEnhancement& enhancement = weapon->get_enhancement();
		if (enhancement.blessing == BlessingStatus::CURSED)
		{
			return -2;
		}
		return enhancement.toHitBonus;
	};

	const Player::DualWieldInfo dualWieldInfo = owner.get_dual_wield_info();
	if (dualWieldInfo.isDualWielding)
	{
		ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, "Dual wielding: ");
		ctx.messageSystem->append_message_part(GREEN_BLACK_PAIR, "Fighting with both weapons!");
		ctx.messageSystem->finalize_message();

		const DamageInfo mainDamage = compute_weapon_damage(EquipmentSlot::RIGHT_HAND);
		Item* mainWeapon = owner.get_equipped_item(EquipmentSlot::RIGHT_HAND);
		const std::string mainName = mainWeapon ? mainWeapon->actorData.name : "unarmed";

		perform_single_attack(
			owner, target, mainDamage,
			dualWieldInfo.mainHandPenalty + weapon_hit_modifier(EquipmentSlot::RIGHT_HAND),
			mainName, ctx);

		if (!target.is_dead())
		{
			const DamageInfo offDamage = compute_weapon_damage(EquipmentSlot::LEFT_HAND);
			perform_single_attack(
				owner, target, offDamage,
				dualWieldInfo.offHandPenalty + weapon_hit_modifier(EquipmentSlot::LEFT_HAND),
				"off hand", ctx);
		}
		return;
	}

	const EquipmentSlot weaponSlot = owner.has_state(ActorState::IS_RANGED)
		? EquipmentSlot::MISSILE_WEAPON
		: EquipmentSlot::RIGHT_HAND;
	Item* weapon = owner.get_equipped_item(weaponSlot);
	const DamageInfo attackDamage = compute_weapon_damage(weaponSlot);
	const std::string weaponName = weapon ? weapon->actorData.name : "unarmed";
	perform_single_attack(owner, target, attackDamage, weapon_hit_modifier(weaponSlot), weaponName, ctx);
}
