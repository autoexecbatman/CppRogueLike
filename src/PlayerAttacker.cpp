#include <string>

#include "Colors.h"
#include "Creature.h"
#include "DamageInfo.h"
#include "GameContext.h"
#include "Item.h"
#include "MessageSystem.h"
#include "Player.h"
#include "PlayerAttacker.h"

PlayerAttacker::PlayerAttacker(Player& owner) :
	Attacker(DamageInfo{}),
	owner(owner)
{
}

AttackResult PlayerAttacker::attack(
	Creature& target,
	AttackKind kind,
	GameContext& ctx)
{
	const Player::DualWieldInfo dualWieldInfo = owner.get_dual_wield_info();
	if (dualWieldInfo.isDualWielding)
	{
		ctx.messageSystem->append_message_part(
			ColorPairId::WHITE_BLACK,
			"Dual wielding: ");
		ctx.messageSystem->append_message_part(
			ColorPairId::GREEN_BLACK,
			"Fighting with both weapons!");
		ctx.messageSystem->finalize_message();

		const Item* mainWeapon =
			owner.get_equipped_item(EquipmentSlot::RIGHT_HAND);
		const DamageInfo mainDamage = weapon_damage(mainWeapon);
		const std::string mainName =
			mainWeapon ? mainWeapon->get_name() : "unarmed";

		const AttackResult mainHand = perform_single_attack(
			owner,
			target,
			mainDamage,
			dualWieldInfo.mainHandPenalty + weapon_hit_modifier(mainWeapon),
			mainName,
			kind,
			ctx);

		// The off hand swings only while the target is still up, so a main hand that
		// killed outright reports for both.
		AttackResult offHand = AttackResult::PREVENTED;
		if (!target.is_dead())
		{
			const Item* offWeapon =
				owner.get_equipped_item(EquipmentSlot::LEFT_HAND);
			offHand = perform_single_attack(
				owner,
				target,
				weapon_damage(offWeapon),
				dualWieldInfo.offHandPenalty + weapon_hit_modifier(offWeapon),
				"off hand",
				kind,
				ctx);
		}
		const bool eitherLanded =
			mainHand == AttackResult::LANDED || offHand == AttackResult::LANDED;
		return eitherLanded ? AttackResult::LANDED : mainHand;
	}

	const Item* weapon = owner.get_equipped_item(weapon_slot_for(kind));
	const std::string weaponName = weapon ? weapon->get_name() : "unarmed";
	return perform_single_attack(
		owner,
		target,
		weapon_damage(weapon),
		weapon_hit_modifier(weapon),
		weaponName,
		kind,
		ctx);
}

// A player's damage comes from the weapon in hand, which the item itself
// serializes, so there is nothing here to write or read back.
void PlayerAttacker::load([[maybe_unused]] const json& source) {}

void PlayerAttacker::save([[maybe_unused]] json& output) {}
