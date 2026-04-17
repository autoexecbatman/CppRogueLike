#include "PlayerAttacker.h"

#include <string>

#include "../Actor/Creature.h"
#include "../Actor/Item.h"
#include "../ActorTypes/Player.h"
#include "../Colors/Colors.h"
#include "../Combat/DamageInfo.h"
#include "../Combat/WeaponDamageRegistry.h"
#include "../Core/GameContext.h"
#include "../Systems/ItemEnhancements/ItemEnhancements.h"
#include "../Systems/MessageSystem.h"

PlayerAttacker::PlayerAttacker(Player& owner)
	: Attacker(DamageInfo{}), owner(owner) {}

DamageInfo PlayerAttacker::compute_weapon_damage(EquipmentSlot slot) const
{
	Item* weapon = owner.get_equipped_item(slot);
	if (weapon && weapon->is_weapon())
	{
		const ItemEnhancement* enhancement = weapon->is_enhanced() ? &weapon->get_enhancement() : nullptr;
		return WeaponDamageRegistry::get_enhanced_damage_info(weapon->item_key, enhancement);
	}
	return WeaponDamageRegistry::get_unarmed_damage_info();
}

void PlayerAttacker::attack(Creature& target, GameContext& ctx)
{
	const Player::DualWieldInfo dualWieldInfo = owner.get_dual_wield_info();
	if (dualWieldInfo.isDualWielding)
	{
		ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, "Dual wielding: ");
		ctx.messageSystem->append_message_part(GREEN_BLACK_PAIR, "Fighting with both weapons!");
		ctx.messageSystem->finalize_message();

		const DamageInfo mainDamage = compute_weapon_damage(EquipmentSlot::RIGHT_HAND);
		Item* mainWeapon = owner.get_equipped_item(EquipmentSlot::RIGHT_HAND);
		const std::string mainName = mainWeapon ? mainWeapon->actorData.name : "unarmed";

		perform_single_attack(owner, target, mainDamage, dualWieldInfo.mainHandPenalty, mainName, ctx);

		if (target.destructible && !target.destructible->is_dead())
		{
			const DamageInfo offDamage = compute_weapon_damage(EquipmentSlot::LEFT_HAND);
			perform_single_attack(owner, target, offDamage, dualWieldInfo.offHandPenalty, "off hand", ctx);
		}
		return;
	}

	const EquipmentSlot weaponSlot = owner.has_state(ActorState::IS_RANGED)
		? EquipmentSlot::MISSILE_WEAPON
		: EquipmentSlot::RIGHT_HAND;
	Item* weapon = owner.get_equipped_item(weaponSlot);
	const DamageInfo attackDamage = compute_weapon_damage(weaponSlot);
	const std::string weaponName = weapon ? weapon->actorData.name : "unarmed";
	perform_single_attack(owner, target, attackDamage, 0, weaponName, ctx);
}
