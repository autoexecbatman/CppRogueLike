#include <algorithm>
#include <cassert>
#include <format>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Actor.h"
#include "AnimationSystem.h"
#include "ArmorClass.h"
#include "AttackStrength.h"
#include "Attacker.h"
#include "BuffSystem.h"
#include "BuffType.h"
#include "Colors.h"
#include "Creature.h"
#include "DamageInfo.h"
#include "DamageResolver.h"
#include "DataManager.h"
#include "DexterityAttributes.h"
#include "DiceExpr.h"
#include "EquipmentSlot.h"
#include "GameContext.h"
#include "Item.h"
#include "LevelUpSystem.h"
#include "MenuTrade.h"
#include "MessageSystem.h"
#include "Persistent.h"
#include "Pickable.h"
#include "Player.h"
#include "TileConfig.h"
#include "Vector2D.h"

// OCP: Data-driven buff break messaging - player notifications when buffs end from attacking
static const std::unordered_map<BuffType, std::string_view> BUFF_BREAK_MESSAGES = {
	{ BuffType::INVISIBILITY, "Your invisibility fades as you attack!" },
	{ BuffType::ITEM_INVISIBILITY, "Your invisibility fades as you attack!" },
	{ BuffType::SANCTUARY, "Your sanctuary is broken by your aggression!" },
};

Attacker::Attacker(const DamageInfo& damage)
	: damageInfo(damage) {}

AttackResult Attacker::perform_single_attack(
	Creature& owner,
	Creature& target,
	const DamageInfo& attackDamage,
	int attackPenalty,
	std::string_view handName,
	AttackKind kind,
	GameContext& ctx)
{
	assert(ctx.dataManager && "Attacker::perform_single_attack: called without a dataManager");
	assert(ctx.dice && "Attacker::perform_single_attack: called without dice");

	// Shopkeeper interaction, melee only: the shop component is what marks a trader.
	if (target.shop && kind == AttackKind::MELEE)
	{
		ctx.menus->push_back(std::make_unique<MenuTrade>(target, owner, ctx));
		return AttackResult::PREVENTED;
	}

	// Every path that builds a creature sets a Strength above zero and nothing in the
	// game lowers a monster's, so a swing without one is a creature left unfinished.
	assert(owner.get_strength() > 0 && "attacked with no Strength");

	// A target already dead is a real outcome: two creatures can swing at one victim
	// in a round, and the second finds it gone.
	if (target.is_dead())
	{
		ctx.messageSystem->append_message_part(owner.actorData.color, owner.actorData.name);
		ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, " attacks ");
		ctx.messageSystem->append_message_part(target.actorData.color, target.actorData.name);
		ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, " in vain.");
		ctx.messageSystem->finalize_message();
		return AttackResult::PREVENTED;
	}

	// A bow made for a stronger arm than this one is not drawn at all, so nothing is rolled
	// against it - not the attack, and not a Sanctuary save the target's ward would ask.
	const Item* missileWeapon = kind == AttackKind::RANGED ? owner.get_equipped_item(EquipmentSlot::MISSILE_WEAPON) : nullptr;
	if (missileWeapon && !can_draw(owner, *missileWeapon))
	{
		ctx.messageSystem->append_message_part(owner.actorData.color, owner.actorData.name);
		ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, " is not strong enough to draw the ");
		ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, missileWeapon->actorData.name);
		ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, ".");
		ctx.messageSystem->finalize_message();
		return AttackResult::PREVENTED;
	}

	// Sanctuary wards whoever bears it: an attacker that fails its save against the
	// target's casting makes no attack on it (PHB page 436).
	if (ctx.buffSystem->is_turned_away_by_sanctuary(owner, target, ctx))
	{
		ctx.messageSystem->append_message_part(owner.actorData.color, owner.actorData.name);
		ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, " cannot bring itself to attack ");
		ctx.messageSystem->append_message_part(target.actorData.color, target.actorData.name);
		ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, ".");
		ctx.messageSystem->finalize_message();
		return AttackResult::PREVENTED;
	}

	// The part of the attacker's Table 1 row this attack takes, by what fires it.
	const AttackStrength::Adjustment strengthOnAttack = AttackStrength::adjustment(
		*ctx.dataManager,
		owner,
		kind,
		missileWeapon);

	const int attackRoll = ctx.dice->d20();

	// A die at a time: fire and cold are resisted per die, and the roll's bonus is not a die.
	// A bite offers no saving throw, so a ring's save bonus adds nothing here.
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

		assert(ctx.animSystem && "Attacker::perform_single_attack: called without an animSystem");
		ctx.animSystem->spawn_melee_hit(target.position);
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
					ColorPairId::CYAN_BLACK,
					std::string(BUFF_BREAK_MESSAGES.at(buff_type)),
					MessageCompletion::FINISHED);
			}
		}

		// Striking from the shadows leaves them, with one line whichever way the secret roll went.
		if (ctx.player_concrete().is_in_shadows())
		{
			ctx.player_concrete().leave_shadows(ctx);
		}
	}

	return isHit ? AttackResult::LANDED : AttackResult::MISSED;
}

BackstabInfo Attacker::calculate_backstab_bonus(const Creature& owner) const noexcept
{
	// AD&D 2e: Invisibility grants backstab - +4 to hit, rogues get damage multiplier
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
	assert(ctx.dataManager && "Attacker::calculate_to_hit_roll: called without a dataManager");

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
			ColorPairId::MAGENTA_BLACK,
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
	std::string_view handName,
	GameContext& ctx) const noexcept
{
	ctx.messageSystem->append_message_part(attacker.actorData.color, attacker.actorData.name);
	ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, std::format(" ({}) rolls ", handName));
	ctx.messageSystem->append_message_part(ColorPairId::GREEN_BLACK, std::format("{}", attackRoll));
	if (attackPenalty != 0)
	{
		ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, std::format(" ({:+})", attackPenalty));
	}
	ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, std::format(" vs {}", rollNeeded));
	ctx.messageSystem->append_message_part(ColorPairId::GREEN_BLACK, ". Hit! ");
	ctx.messageSystem->append_message_part(target.actorData.color, target.actorData.name);
	ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, " takes ");
	ctx.messageSystem->append_message_part(ColorPairId::RED_BLACK, std::format("{}", finalDamage));
	ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, std::format(" dmg ({}).", attackDamage.displayRoll));
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
	std::string_view handName,
	GameContext& ctx) const noexcept
{
	ctx.messageSystem->append_message_part(attacker.actorData.color, attacker.actorData.name);
	ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, std::format(" ({}) rolls ", handName));
	ctx.messageSystem->append_message_part(ColorPairId::RED_BLACK, std::format("{}", attackRoll));
	if (attackPenalty != 0)
	{
		ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, std::format(" ({:+})", attackPenalty));
	}
	ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, std::format(" vs {}", rollNeeded));
	ctx.messageSystem->append_message_part(ColorPairId::RED_BLACK, ". Miss! ");
	ctx.messageSystem->append_message_part(target.actorData.color, target.actorData.name);
	ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, " is unharmed.");
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
	const json& damageJson = j.at("damageInfo");
	damageInfo = DamageInfo{
		damageJson.at("display").get<std::string>(),
		parse_damage_type(damageJson.at("type").get<std::string>())
	};
}

void Attacker::save(json& j)
{
	j["damageInfo"]["display"] = damageInfo.displayRoll;
	j["damageInfo"]["type"] = damage_type_name(damageInfo.damageType);
}

ShotImpact RangedShot::impact_of(AttackResult result) noexcept
{
	switch (result)
	{
	case AttackResult::LANDED:
	{
		return ShotImpact::MISSILE_AND_BLOOD;
	}
	case AttackResult::MISSED:
	{
		return ShotImpact::MISSILE;
	}
	case AttackResult::PREVENTED:
	{
		return ShotImpact::NOTHING;
	}
	}

	std::unreachable();
}

AttackResult RangedShot::fire(Creature& shooter, Creature& target, GameContext& ctx)
{
	assert(shooter.attacker && "RangedShot::fire: the shooter has no attacker");
	assert(ctx.animSystem && "RangedShot::fire: called without an animSystem");
	assert(ctx.tileConfig && "RangedShot::fire: called without a tileConfig");

	// The attack resolves as the missile leaves, so its messages print before it lands.
	const AttackResult result = shooter.attacker->attack(target, AttackKind::RANGED, ctx);
	const ShotImpact impact = impact_of(result);
	if (impact == ShotImpact::NOTHING)
	{
		return result;
	}

	// A missile with no arrival callback is dropped at once, so a miss carries an empty one.
	const Vector2D landing = target.position;
	const bool isBloody = impact == ShotImpact::MISSILE_AND_BLOOD;
	AnimationSystem& animSystem = *ctx.animSystem;
	const auto on_arrive = [landing, isBloody, &animSystem]()
	{
		if (isBloody)
		{
			animSystem.spawn_blood_burst(landing, 3);
		}
	};
	animSystem.spawn_projectile(
		shooter.position,
		landing,
		ctx.tileConfig->get("TILE_EFFECT_BOLT"),
		210,
		180,
		100,
		550.0f,
		0.0f,
		on_arrive);
	return result;
}
