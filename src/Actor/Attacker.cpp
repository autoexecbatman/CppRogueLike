#include <format>
#include <memory>
#include <unordered_map>
#include <string_view>

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
#include "../Systems/BuffSystem.h"
#include "../Combat/WeaponDamageRegistry.h"
#include "../Actor/Destructible.h"

// OCP: Data-driven buff break messaging - player notifications when buffs end from attacking
static const std::unordered_map<BuffType, std::string_view> buff_break_messages = {
	{BuffType::INVISIBILITY, "Your invisibility fades as you attack!"},
	// Future extensions: {BuffType::SANCTUARY, "Your sanctuary is broken by your aggression!"},
};

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
		const EquipmentSlot weaponSlot = attacker.has_state(ActorState::IS_RANGED)
			? EquipmentSlot::MISSILE_WEAPON
			: EquipmentSlot::RIGHT_HAND;
		Item* weapon = player->get_equipped_item(weaponSlot);
		std::string weaponName = weapon ? weapon->actorData.name : "unarmed";
		perform_single_attack(attacker, target, 0, weaponName, ctx);
		return;
	}

	// Monster attack - use stored weapon name
	perform_single_attack(attacker, target, 0, attacker.get_weapon_equipped(), ctx);
}

void Attacker::perform_single_attack(
	Creature& attacker,
	Creature& target,
	int attackPenalty,
	const std::string& handName,
	GameContext& ctx
)
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

	// Get damage and roll dice
	const DamageInfo attackDamage = get_attack_damage(attacker);
	const int attackRoll = ctx.dice->d20();
	const int damageRoll = ctx.dice->roll(attackDamage.minDamage, attackDamage.maxDamage);

	// Calculate backstab and to-hit roll
	const BackstabInfo backstab = calculate_backstab_bonus(attacker);
	const int rollNeeded = calculate_to_hit_roll(attacker, target, attackPenalty, backstab, ctx);
	const bool isHit = (attackRoll >= rollNeeded);

	if (isHit)
	{
		const int baseDamage = calculate_damage_with_backstab(damageRoll, strengthAttr.dmgAdj, backstab, ctx);
		const int finalDamage = std::max(0, baseDamage - target.destructible->get_dr());

		log_attack_hit(
			attacker,
			target,
			attackRoll,
			rollNeeded,
			attackPenalty,
			finalDamage,
			damageRoll,
			attackDamage,
			strengthAttr.dmgAdj,
			target.destructible->get_dr(),
			handName,
			ctx
		);

		if (finalDamage > 0)
		{
			target.destructible->take_damage(target, finalDamage, ctx, attackDamage.damageType);
		}
	}
	else
	{
		log_attack_miss(
			attacker,
			target,
			attackRoll,
			rollNeeded,
			attackPenalty,
			handName,
			ctx
		);
	}

	// AD&D 2e: Remove buffs that break when attacking (Invisibility, Sanctuary, etc.) - OCP compliant
	const auto broken_buffs = ctx.buff_system->remove_buffs_broken_by_attacking(attacker);

	// Show player notification for broken buffs
	if (attacker.uniqueId == ctx.player->uniqueId)
	{
		for (BuffType buff_type : broken_buffs)
		{
			if (buff_break_messages.contains(buff_type))
			{
				ctx.message_system->message(
					CYAN_BLACK_PAIR,
					std::string(buff_break_messages.at(buff_type)),
					true
				);
			}
		}
	}
}

BackstabInfo Attacker::calculate_backstab_bonus(Creature& attacker) const noexcept
{
	// AD&D 2e: Invisibility grants backstab - +4 to hit, rogues get damage multiplier
	if (!attacker.is_invisible())
	{
		return BackstabInfo{ false, 0, 1 };
	}

	BackstabInfo info{ };
	info.isBackstab = true;
	info.hitBonus = 4; // +4 to hit from behind/invisible
	info.damageMultiplier = 1;

	auto* player = dynamic_cast<Player*>(&attacker);
	if (player && player->playerClassState == Player::PlayerClassState::ROGUE)
	{
		info.damageMultiplier = LevelUpSystem::calculate_backstab_multiplier(player->get_player_level());
	}

	return info;
}

int Attacker::calculate_to_hit_roll(
	const Creature& attacker,
	const Creature& target,
	int attackPenalty,
	const BackstabInfo& backstab,
	GameContext& ctx
) const noexcept
{
	// AD&D 2e: THAC0 - AC = roll needed
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
				ctx.message_system->log(std::format(
					"Ranged modifier: {} from DEX {}",
					dexAttr.MissileAttackAdj,
					attacker.get_dexterity()
				));
			}
		}
	}

	// AD&D 2e: Add all buff-based hit modifiers (Bless, Prayer, etc.) - OCP compliant
	hitModifier += ctx.buff_system->calculate_hit_modifier(attacker);

	// AD&D 2e: Add backstab bonus
	hitModifier += backstab.hitBonus;

	return rollNeeded - hitModifier;
}

int Attacker::calculate_damage_with_backstab(
	int damageRoll,
	int strengthBonus,
	const BackstabInfo& backstab,
	GameContext& ctx
) const noexcept
{
	int baseDamage = damageRoll + strengthBonus;

	if (backstab.isBackstab && backstab.damageMultiplier > 1)
	{
		baseDamage *= backstab.damageMultiplier;
		ctx.message_system->append_message_part(
			MAGENTA_BLACK_PAIR,
			std::format(" BACKSTAB x{}! ", backstab.damageMultiplier)
		);
	}

	return baseDamage;
}

void Attacker::log_attack_hit(
	const Creature& attacker,
	const Creature& target,
	int attackRoll,
	int rollNeeded,
	int attackPenalty,
	int finalDamage,
	int damageRoll,
	const DamageInfo& attackDamage,
	int strengthBonus,
	int dr,
	const std::string& handName,
	GameContext& ctx
) const noexcept
{
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
		damageRoll, attackDamage.get_damage_range(), strengthBonus, dr, finalDamage
	));
}

void Attacker::log_attack_miss(
	const Creature& attacker,
	const Creature& target,
	int attackRoll,
	int rollNeeded,
	int attackPenalty,
	const std::string& handName,
	GameContext& ctx
) const noexcept
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
		handName,
		attacker.actorData.name,
		attackRoll,
		rollNeeded,
		attacker.destructible->get_thaco(),
		target.destructible->get_armor_class(),
		attackPenalty
	));
}

DamageInfo Attacker::get_attack_damage(Creature& attacker) const
{
	auto* player = dynamic_cast<Player*>(&attacker);
	if (player)
	{
		const EquipmentSlot weaponSlot = attacker.has_state(ActorState::IS_RANGED)
			? EquipmentSlot::MISSILE_WEAPON
			: EquipmentSlot::RIGHT_HAND;
		Item* weapon = player->get_equipped_item(weaponSlot);
		if (weapon && weapon->is_weapon())
		{
			const ItemEnhancement* enhancement = weapon->is_enhanced() ? &weapon->get_enhancement() : nullptr;
			return WeaponDamageRegistry::get_enhanced_damage_info(weapon->itemId, enhancement);
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
	damageInfo.damageType = static_cast<DamageType>(j["damageInfo"]["type"]);
}

void Attacker::save(json& j)
{
	j["damageInfo"]["min"] = damageInfo.minDamage;
	j["damageInfo"]["max"] = damageInfo.maxDamage;
	j["damageInfo"]["display"] = damageInfo.displayRoll;
	j["damageInfo"]["type"] = static_cast<int>(damageInfo.damageType);
}
