#include <string>

#include "Creature.h"
#include "Item.h"
#include "Player.h"
#include "Colors.h"
#include "DamageInfo.h"
#include "WeaponDamageRegistry.h"
#include "GameContext.h"
#include "ItemIdentification.h"
#include "ItemEnhancements.h"
#include "MessageSystem.h"
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

AttackResult PlayerAttacker::attack(Creature& target, AttackKind kind, GameContext& ctx)
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

		const AttackResult mainHand = perform_single_attack(
			owner, target, mainDamage,
			dualWieldInfo.mainHandPenalty + weapon_hit_modifier(EquipmentSlot::RIGHT_HAND),
			mainName, kind, ctx);

		// The off hand swings only while the target is still up, so a main hand that
		// killed outright reports for both.
		AttackResult offHand = AttackResult::PREVENTED;
		if (!target.is_dead())
		{
			const DamageInfo offDamage = compute_weapon_damage(EquipmentSlot::LEFT_HAND);
			offHand = perform_single_attack(
				owner, target, offDamage,
				dualWieldInfo.offHandPenalty + weapon_hit_modifier(EquipmentSlot::LEFT_HAND),
				"off hand", kind, ctx);
		}
		const bool eitherLanded = mainHand == AttackResult::LANDED || offHand == AttackResult::LANDED;
		return eitherLanded ? AttackResult::LANDED : mainHand;
	}

	// The attack says which weapon it is made with. Carrying a bow does not change
	// what is in the attacker's hand when they swing.
	const EquipmentSlot weaponSlot = kind == AttackKind::RANGED
		? EquipmentSlot::MISSILE_WEAPON
		: EquipmentSlot::RIGHT_HAND;
	Item* weapon = owner.get_equipped_item(weaponSlot);
	const DamageInfo attackDamage = compute_weapon_damage(weaponSlot);
	const std::string weaponName = weapon ? weapon->actorData.name : "unarmed";
	return perform_single_attack(owner, target, attackDamage, weapon_hit_modifier(weaponSlot), weaponName, kind, ctx);
}

// A player's damage comes from the weapon in hand, which the item itself
// serializes, so there is nothing here to write or read back.
void PlayerAttacker::load(const json& j)
{
}

void PlayerAttacker::save(json& j)
{
}
