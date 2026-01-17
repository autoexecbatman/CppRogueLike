#include <format>
#include <memory>

#include "../Core/GameContext.h"
#include "../Colors/Colors.h"
#include "../Actor/Actor.h"
#include "../ActorTypes/Player.h"
#include "../Menu/MenuTrade.h"
#include "Attacker.h"
#include "../Ai/AiShopkeeper.h"
#include "../Attributes/DexterityAttributes.h"
#include "../Systems/MessageSystem.h"
#include "../Systems/DataManager.h"
#include "../Systems/LevelUpSystem.h"

Attacker::Attacker(const DamageInfo& damage) : damageInfo(damage) {}

void Attacker::attack(Creature& attacker, Creature& target, GameContext& ctx)
{
	auto* player = dynamic_cast<Player*>(&attacker);
	if (player)
	{
		// Check for dual wielding
		auto dualWieldInfo = player->get_dual_wield_info();
		if (dualWieldInfo.isDualWielding)
		{
			ctx.message_system->append_message_part(WHITE_BLACK_PAIR, "Dual wielding: ");
			ctx.message_system->append_message_part(GREEN_BLACK_PAIR, "Fighting with both weapons!");
			ctx.message_system->finalize_message();

			perform_single_attack(attacker, target, dualWieldInfo.mainHandPenalty, "main hand", ctx);

			if (target.destructible && !target.destructible->is_dead())
			{
				perform_single_attack(attacker, target, dualWieldInfo.offHandPenalty, "off hand", ctx);
			}
			return;
		}

		// Single source of truth: derive weapon name from equipped item
		Item* weapon = player->get_equipped_item(EquipmentSlot::RIGHT_HAND);
		std::string weaponName = weapon ? weapon->actorData.name : "unarmed";
		perform_single_attack(attacker, target, 0, weaponName, ctx);
		return;
	}

	// Monster attack - use stored weapon name
	perform_single_attack(attacker, target, 0, attacker.get_weapon_equipped(), ctx);
}

void Attacker::perform_single_attack(Creature& attacker, Creature& target, int attackPenalty, const std::string& handName, GameContext& ctx)
{
	if (!target.destructible)
	{
		ctx.message_system->log("WARNING: Target has no destructible component");
		return;
	}

	// Shopkeeper interaction (melee only)
	if (dynamic_cast<AiShopkeeper*>(target.ai.get()) && !attacker.has_state(ActorState::IS_RANGED))
	{
		ctx.menus->push_back(std::make_unique<MenuTrade>(target, attacker, ctx));
		return;
	}

	// Cannot attack dead targets or without strength
	if (target.destructible->is_dead() || attacker.get_strength() <= 0)
	{
		ctx.message_system->append_message_part(attacker.actorData.color, attacker.actorData.name);
		ctx.message_system->append_message_part(WHITE_BLACK_PAIR, " attacks ");
		ctx.message_system->append_message_part(target.actorData.color, target.actorData.name);
		ctx.message_system->append_message_part(WHITE_BLACK_PAIR, " in vain.");
		ctx.message_system->finalize_message();
		return;
	}

	// Validate strength attribute
	const int strIndex = attacker.get_strength() - 1;
	if (strIndex < 0 || static_cast<size_t>(strIndex) >= ctx.data_manager->get_strength_attributes().size())
	{
		ctx.message_system->log(std::format("ERROR: Invalid strength {} for {}", attacker.get_strength(), attacker.actorData.name));
		return;
	}
	const auto& strengthAttr = ctx.data_manager->get_strength_attributes().at(strIndex);

	// Get damage and roll
	const DamageInfo attackDamage = get_attack_damage(attacker);
	const int attackRoll = ctx.dice->d20();
	const int damageRoll = attackDamage.roll_damage();

	// THAC0 calculation
	int rollNeeded = attacker.destructible->get_thaco() - target.destructible->get_armor_class();
	int hitModifier = attackPenalty;

	// Ranged: apply dexterity modifier
	if (attacker.has_state(ActorState::IS_RANGED))
	{
		const int dexIndex = attacker.get_dexterity() - 1;
		if (dexIndex >= 0 && static_cast<size_t>(dexIndex) < ctx.data_manager->get_dexterity_attributes().size())
		{
			const auto& dexAttr = ctx.data_manager->get_dexterity_attributes().at(dexIndex);
			hitModifier += dexAttr.MissileAttackAdj;

			if (dexAttr.MissileAttackAdj != 0)
			{
				ctx.message_system->log(std::format("Ranged modifier: {} from DEX {}", dexAttr.MissileAttackAdj, attacker.get_dexterity()));
			}
		}
	}

	// Backstab: invisible attackers get +4 to hit and damage multiplier (rogues)
	int backstabMultiplier = 1;
	bool isBackstab = false;
	if (attacker.is_invisible())
	{
		isBackstab = true;
		hitModifier += 4; // +4 to hit from behind/invisible

		auto* player = dynamic_cast<Player*>(&attacker);
		if (player && player->playerClassState == Player::PlayerClassState::ROGUE)
		{
			backstabMultiplier = LevelUpSystem::calculate_backstab_multiplier(player->get_player_level());
		}
		// Break invisibility after attack
		attacker.clear_invisible();
	}

	rollNeeded -= hitModifier;
	const bool isHit = (attackRoll >= rollNeeded);

	if (isHit)
	{
		int baseDamage = damageRoll + strengthAttr.dmgAdj;
		if (isBackstab && backstabMultiplier > 1)
		{
			baseDamage *= backstabMultiplier;
			ctx.message_system->append_message_part(MAGENTA_BLACK_PAIR, std::format(" BACKSTAB x{}! ", backstabMultiplier));
		}
		const int finalDamage = std::max(0, baseDamage - target.destructible->get_dr());

		ctx.message_system->append_message_part(attacker.actorData.color, attacker.actorData.name);
		ctx.message_system->append_message_part(WHITE_BLACK_PAIR, std::format(" ({}) rolls ", handName));
		ctx.message_system->append_message_part(GREEN_BLACK_PAIR, std::format("{}", attackRoll));
		if (attackPenalty != 0)
		{
			ctx.message_system->append_message_part(WHITE_BLACK_PAIR, std::format(" ({})", attackPenalty));
		}
		ctx.message_system->append_message_part(WHITE_BLACK_PAIR, std::format(" vs {}", rollNeeded));
		ctx.message_system->append_message_part(GREEN_BLACK_PAIR, ". Hit! ");
		ctx.message_system->append_message_part(RED_BLACK_PAIR, std::format("{}", finalDamage));
		ctx.message_system->append_message_part(WHITE_BLACK_PAIR, std::format(" dmg ({}).", attackDamage.displayRoll));
		ctx.message_system->finalize_message();

		ctx.message_system->log(std::format(
			"HIT ({}): {} rolled {} vs {} | {} ({}) + {} str - {} DR = {} dmg",
			handName, attacker.actorData.name, attackRoll, rollNeeded,
			damageRoll, attackDamage.get_damage_range(), strengthAttr.dmgAdj,
			target.destructible->get_dr(), finalDamage
		));

		if (finalDamage > 0)
		{
			target.destructible->take_damage(target, finalDamage, ctx);
		}
	}
	else
	{
		ctx.message_system->append_message_part(attacker.actorData.color, attacker.actorData.name);
		ctx.message_system->append_message_part(WHITE_BLACK_PAIR, std::format(" ({}) rolls ", handName));
		ctx.message_system->append_message_part(RED_BLACK_PAIR, std::format("{}", attackRoll));
		if (attackPenalty != 0)
		{
			ctx.message_system->append_message_part(WHITE_BLACK_PAIR, std::format(" ({})", attackPenalty));
		}
		ctx.message_system->append_message_part(WHITE_BLACK_PAIR, std::format(" vs {}", rollNeeded));
		ctx.message_system->append_message_part(RED_BLACK_PAIR, ". Miss!");
		ctx.message_system->finalize_message();

		ctx.message_system->log(std::format(
			"MISS ({}): {} rolled {} vs {} (THAC0:{}, AC:{}, Penalty:{})",
			handName, attacker.actorData.name, attackRoll, rollNeeded,
			attacker.destructible->get_thaco(), target.destructible->get_armor_class(), attackPenalty
		));
	}
}

DamageInfo Attacker::get_attack_damage(Creature& attacker) const
{
	auto* player = dynamic_cast<Player*>(&attacker);
	if (player)
	{
		Item* weapon = player->get_equipped_item(EquipmentSlot::RIGHT_HAND);
		if (weapon && weapon->is_weapon())
		{
			const ItemEnhancement* enhancement = weapon->is_enhanced() ? &weapon->get_enhancement() : nullptr;
			return WeaponDamageRegistry::get_enhanced_damage_info(weapon->itemClass, enhancement);
		}
		return WeaponDamageRegistry::get_unarmed_damage_info();
	}

	return damageInfo;
}

void Attacker::load(const json& j)
{
	damageInfo.minDamage = j["damageInfo"]["min"];
	damageInfo.maxDamage = j["damageInfo"]["max"];
	damageInfo.displayRoll = j["damageInfo"]["display"];
}

void Attacker::save(json& j)
{
	j["damageInfo"]["min"] = damageInfo.minDamage;
	j["damageInfo"]["max"] = damageInfo.maxDamage;
	j["damageInfo"]["display"] = damageInfo.displayRoll;
}
