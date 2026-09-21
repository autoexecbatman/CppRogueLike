#include <vector>
#include <algorithm>
#include <format>
#include <memory>
#include <string>
#include <unordered_map>

#include "Actor.h"
#include "ArmorClass.h"
#include "Creature.h"
#include "DexterityAttributes.h"
#include "Colors.h"
#include "DamageInfo.h"
#include "GameContext.h"
#include "MenuTrade.h"
#include "Persistent.h"
#include "AnimationSystem.h"
#include "BuffSystem.h"
#include "BuffType.h"
#include "DataManager.h"
#include "LevelUpSystem.h"
#include "MessageSystem.h"
#include "StrengthAttributes.h"
#include "Attacker.h"
#include "AttackStrength.h"
#include "DamageResolver.h"
#include "DiceExpr.h"
#include "EquipmentSlot.h"
#include "Item.h"

// OCP: Data-driven buff break messaging - player notifications when buffs end from attacking
static const std::unordered_map<BuffType, std::string_view> BUFF_BREAK_MESSAGES = {
	{ BuffType::INVISIBILITY, "Your invisibility fades as you attack!" },
	{ BuffType::SANCTUARY, "Your sanctuary is broken by your aggression!" },
};

Attacker::Attacker(const DamageInfo& damage)
	: damageInfo(damage) {}

void Attacker::perform_single_attack(
	Creature& owner,
	Creature& target,
	const DamageInfo& attackDamage,
	int attackPenalty,
	const std::string& handName,
	AttackKind kind,
	GameContext& ctx)
{
	// Shopkeeper interaction, melee only: the shop component is what marks a trader.
	if (target.shop && kind == AttackKind::MELEE)
	{
		ctx.menus->push_back(std::make_unique<MenuTrade>(target, owner, ctx));
		return;
	}

	// Cannot attack dead targets or without strength
	if (target.is_dead() || owner.get_strength() <= 0)
	{
		ctx.messageSystem->append_message_part(owner.actorData.color, owner.actorData.name);
		ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, " attacks ");
		ctx.messageSystem->append_message_part(target.actorData.color, target.actorData.name);
		ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, " in vain.");
		ctx.messageSystem->finalize_message();
		return;
	}

	// Sanctuary wards whoever bears it: an attacker that fails its save against the
	// target's casting makes no attack on it (PHB page 436).
	if (ctx.buffSystem->is_turned_away_by_sanctuary(owner, target, ctx))
	{
		ctx.messageSystem->append_message_part(owner.actorData.color, owner.actorData.name);
		ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, " cannot bring itself to attack ");
		ctx.messageSystem->append_message_part(target.actorData.color, target.actorData.name);
		ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, ".");
		ctx.messageSystem->finalize_message();
		return;
	}

	// The part of the attacker's Table 1 row this attack takes, by what fires it.
	const Item* missileWeapon = kind == AttackKind::RANGED ? owner.get_equipped_item(EquipmentSlot::MISSILE_WEAPON) : nullptr;
	const AttackStrength::Adjustment strengthOnAttack = AttackStrength::adjustment(
		*ctx.dataManager,
		owner.get_strength(),
		owner.get_exceptional_strength(),
		kind,
		missileWeapon);

	const int attackRoll = ctx.dice->d20();

	// A die at a time: fire and cold are resisted on each die by the book, and
	// the bonus on the roll is not a die. A bite offers no saving throw, so the
	// ring's save bonus has nothing to add here.
	const std::vector<int> dice = roll_each_die(ctx.dice, attackDamage.dice);
	const int strength = DamageResolver::resistance_strength(attackDamage.damageType, target, ctx);
	const DamageResolver::ResistedDamage reduced = DamageResolver::reduce_dice(dice, attackDamage.damageType, strength);
	const int damageRoll = reduced.hit_points() + attackDamage.dice.bonus;

	// Calculate backstab and to-hit roll
	const BackstabInfo backstab = calculate_backstab_bonus(owner);
	const int rollNeeded = calculate_to_hit_roll(owner, target, attackPenalty, strengthOnAttack.hit, backstab, kind, ctx);
	const bool isHit = (attackRoll >= rollNeeded);

	if (isHit)
	{
		// A blow that lands always tells: "regardless of subtractions, a successful attack
		// roll can never cause less than 1 point of damage" (Player's Handbook, PDF page 29).
		const int baseDamage = calculate_damage_with_backstab(damageRoll, strengthOnAttack.damage, backstab, ctx);
		const int finalDamage = std::max(1, baseDamage - target.get_dr());

		log_attack_hit(
			owner,
			target,
			attackRoll,
			rollNeeded,
			attackPenalty,
			finalDamage,
			damageRoll,
			attackDamage,
			strengthOnAttack.damage,
			target.get_dr(),
			handName,
			ctx);

		if (ctx.animSystem)
		{
			ctx.animSystem->spawn_melee_hit(target.position);
		}
		// The reduced roll, carried on to the number the bonus, strength and damage
		// reduction made of it.
		target.take_damage_and_check_death(reduced.at(finalDamage), ctx);
	}
	else
	{
		log_attack_miss(
			owner,
			target,
			attackRoll,
			rollNeeded,
			attackPenalty,
			handName,
			ctx);
	}

	// AD&D 2e: Remove buffs that break when attacking (Invisibility, Sanctuary, etc.) - OCP compliant
	const auto broken_buffs = ctx.buffSystem->remove_buffs_broken_by_attacking(owner);

	// Show player notification for broken buffs
	if (owner.is_player())
	{
		for (BuffType buff_type : broken_buffs)
		{
			if (BUFF_BREAK_MESSAGES.contains(buff_type))
			{
				ctx.messageSystem->message(
					CYAN_BLACK_PAIR,
					std::string(BUFF_BREAK_MESSAGES.at(buff_type)),
					true);
			}
		}
	}
}

BackstabInfo Attacker::calculate_backstab_bonus(const Creature& owner) const noexcept
{
	// AD&D 2e: Invisibility grants backstab — +4 to hit, rogues get damage multiplier
	if (!owner.is_invisible())
	{
		return BackstabInfo{ false, 0, 1 };
	}

	BackstabInfo info{};
	info.isBackstab = true;
	info.hitBonus = 4; // +4 to hit from behind/invisible
	info.damageMultiplier = 1;

	// Backstab is a rogue ability, so the attacker's class decides it.
	if (owner.get_creature_class() == CreatureClass::ROGUE)
	{
		info.damageMultiplier = LevelUpSystem::calculate_backstab_multiplier(owner.get_creature_level());
	}

	return info;
}

int Attacker::calculate_to_hit_roll(
	const Creature& attacker,
	const Creature& target,
	int attackPenalty,
	int strengthHit,
	const BackstabInfo& backstab,
	AttackKind kind,
	GameContext& ctx) const noexcept
{
	// AD&D 2e: THAC0 - AC = roll needed
	int rollNeeded = attacker.get_thaco() - armor_class_attacked(target, ctx);
	int hitModifier = attackPenalty + strengthHit;

	// Table 51: a surprised defender is +1 to hit.
	if (target.has_state(ActorState::IS_SURPRISED))
	{
		hitModifier += 1;
	}

	// A missile attack takes the dexterity missile adjustment; a swing does not.
	if (kind == AttackKind::RANGED)
	{
		const int missileAdjustment = ctx.dataManager->dexterity_for(attacker.get_dexterity()).MissileAttackAdj;
		hitModifier += missileAdjustment;

		if (missileAdjustment != 0)
		{
			ctx.messageSystem->log(std::format(
				"Ranged modifier: {} from DEX {}",
				missileAdjustment,
				attacker.get_dexterity()));
		}
	}

	// AD&D 2e: Add all buff-based hit modifiers (Bless, Prayer, etc.) - OCP compliant
	hitModifier += ctx.buffSystem->calculate_hit_modifier(attacker);

	// The target's own wards can penalise this attacker, depending on who it is.
	hitModifier += ctx.buffSystem->calculate_ward_penalty(attacker, target);

	// AD&D 2e: Add backstab bonus
	hitModifier += backstab.hitBonus;

	return rollNeeded - hitModifier;
}

int Attacker::calculate_damage_with_backstab(
	int damageRoll,
	int strengthBonus,
	const BackstabInfo& backstab,
	GameContext& ctx) const noexcept
{
	int baseDamage = damageRoll + strengthBonus;

	if (backstab.isBackstab && backstab.damageMultiplier > 1)
	{
		baseDamage *= backstab.damageMultiplier;
		ctx.messageSystem->append_message_part(
			MAGENTA_BLACK_PAIR,
			std::format(" BACKSTAB x{}! ", backstab.damageMultiplier));
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
	GameContext& ctx) const noexcept
{
	ctx.messageSystem->append_message_part(attacker.actorData.color, attacker.actorData.name);
	ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, std::format(" ({}) rolls ", handName));
	ctx.messageSystem->append_message_part(GREEN_BLACK_PAIR, std::format("{}", attackRoll));
	if (attackPenalty != 0)
	{
		ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, std::format(" ({:+})", attackPenalty));
	}
	ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, std::format(" vs {}", rollNeeded));
	ctx.messageSystem->append_message_part(GREEN_BLACK_PAIR, ". Hit! ");
	ctx.messageSystem->append_message_part(target.actorData.color, target.actorData.name);
	ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, " takes ");
	ctx.messageSystem->append_message_part(RED_BLACK_PAIR, std::format("{}", finalDamage));
	ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, std::format(" dmg ({}).", attackDamage.displayRoll));
	ctx.messageSystem->finalize_message();

	ctx.messageSystem->log(std::format(
		"HIT ({}): {} rolled {} vs {} | {} ({}) + {} str - {} DR = {} dmg",
		handName,
		attacker.actorData.name,
		attackRoll,
		rollNeeded,
		damageRoll,
		attackDamage.get_damage_range(),
		strengthBonus,
		dr,
		finalDamage));
}

void Attacker::log_attack_miss(
	const Creature& attacker,
	const Creature& target,
	int attackRoll,
	int rollNeeded,
	int attackPenalty,
	const std::string& handName,
	GameContext& ctx) const noexcept
{
	ctx.messageSystem->append_message_part(attacker.actorData.color, attacker.actorData.name);
	ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, std::format(" ({}) rolls ", handName));
	ctx.messageSystem->append_message_part(RED_BLACK_PAIR, std::format("{}", attackRoll));
	if (attackPenalty != 0)
	{
		ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, std::format(" ({:+})", attackPenalty));
	}
	ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, std::format(" vs {}", rollNeeded));
	ctx.messageSystem->append_message_part(RED_BLACK_PAIR, ". Miss! ");
	ctx.messageSystem->append_message_part(target.actorData.color, target.actorData.name);
	ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, " is unharmed.");
	ctx.messageSystem->finalize_message();

	ctx.messageSystem->log(std::format(
		"MISS ({}): {} rolled {} vs {} (THAC0:{}, AC:{}, Penalty:{})",
		handName,
		attacker.actorData.name,
		attackRoll,
		rollNeeded,
		attacker.get_thaco(),
		armor_class_attacked(target, ctx),
		attackPenalty));
}

int Attacker::armor_class_attacked(const Creature& target, GameContext& ctx) const
{
	// Surprise costs the defender any Dexterity bonus for the instant it lasts.
	if (target.has_state(ActorState::IS_SURPRISED))
	{
		return target.armorClass->without_dexterity_bonus(target, ctx);
	}
	return target.get_armor_class();
}

void Attacker::load(const json& j)
{
	// The dice are the record; the range is derived from them.
	damageInfo = DamageInfo{
		j["damageInfo"]["display"].get<std::string>(),
		static_cast<DamageType>(j["damageInfo"]["type"].get<int>())
	};
}

void Attacker::save(json& j)
{
	j["damageInfo"]["display"] = damageInfo.displayRoll;
	j["damageInfo"]["type"] = static_cast<int>(damageInfo.damageType);
}
