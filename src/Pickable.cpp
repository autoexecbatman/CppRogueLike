// file: Pickable.cpp
//
// What every kind of item does when it is used.
//
// An item's behaviour is a std::variant - ItemBehavior in Pickable.h - and this file
// holds one `use` overload per alternative in it: a potion, a scroll, a weapon, a
// corpse, gold. Nothing switches on a type tag; the variant visitor picks the overload,
// so adding a behaviour means adding an alternative and an overload and the compiler
// finds every place that must handle it.
//
// Each overload returns whether the item was spent. That is what tells the caller to
// remove it from the pack, so a use that failed must return false or the item vanishes
// for nothing.
//
// Usage:
//
//   std::visit([&](auto& behavior) { return use(behavior, item, wearer, ctx); },
//              item.behavior);        // -> true when the item was consumed
//
// Items reach the pack already identified or not, and several overloads read that:
// an unidentified potion announces itself by what it does rather than by its name.

#include <algorithm>
#include <cassert>
#include <format>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>

#include "DamageInfo.h"
#include "SavingThrow.h"
#include "VariantVisitor.h"

#include "Actor.h"
#include "BuffSystem.h"
#include "BuffType.h"
#include "Colors.h"
#include "Creature.h"
#include "CreatureManager.h"
#include "EquipmentSlot.h"
#include "FloatingTextSystem.h"
#include "GameContext.h"
#include "HungerSystem.h"
#include "InventoryData.h"
#include "InventoryOperations.h"
#include "MagicalItemEffects.h"
#include "Map.h"
#include "MessageSystem.h"
#include "Persistent.h"
#include "Pickable.h"
#include "Player.h"
#include "SpawnUtils.h"
#include "SpellAnimations.h"
#include "SpellSystem.h"
#include "TargetMode.h"
#include "TargetingMenu.h"
#include "TargetingSystem.h"
#include "Vector2D.h"
#include "Weapons.h"

// ========== Internal helpers ==========

namespace
{

bool consume_item(Item& owner, Creature& wearer)
{
	auto result = InventoryOperations::remove_item(wearer.inventoryData, owner);
	return result.has_value();
}

const std::unordered_map<std::string, int> corpseNutritionValues = {
	{ "dead goblin", 40 },
	{ "dead orc", 80 },
	{ "dead troll", 120 },
	{ "dead dragon", 200 },
	{ "dead archer", 70 },
	{ "dead mage", 60 },
	{ "dead shopkeeper", 100 },
};

const std::unordered_map<std::string, std::string> corpseFlavorText = {
	{ "dead goblin", "It's greasy and gamey." },
	{ "dead orc", "It's tough and stringy." },
	{ "dead troll", "It's surprisingly filling, if you can stomach it." },
	{ "dead dragon", "It tastes exotic and somewhat spicy!" },
	{ "dead archer", "It tastes... questionable." },
	{ "dead mage", "There's a strange aftertaste of magical residue." },
	{ "dead shopkeeper", "Well-marbled, but you feel guilty..." },
};

template <typename K, typename V>
const V& get_or_default(const std::unordered_map<K, V>& map, const K& key, const V& default_value) noexcept
{
	return map.contains(key) ? map.at(key) : default_value;
}

// Shared save for stat-boost equipment (Gauntlets, Girdle, JewelryAmulet)
template <typename T>
void save_stat_boost(const T& statBoost, PickableType type, json& output)
{
	output["type"] = encode_pickable_type(type);
	output["strBonus"] = statBoost.strBonus;
	output["dexBonus"] = statBoost.dexBonus;
	output["conBonus"] = statBoost.conBonus;
	output["intBonus"] = statBoost.intBonus;
	output["wisBonus"] = statBoost.wisBonus;
	output["chaBonus"] = statBoost.chaBonus;
	output["isSetMode"] = statBoost.isSetMode;
	output["exceptionalStrength"] = statBoost.exceptionalStrength;
}

template <typename T>
void load_stat_boost(T& statBoost, const json& source)
{
	statBoost.strBonus = source.at("strBonus").get<int>();
	statBoost.dexBonus = source.at("dexBonus").get<int>();
	statBoost.conBonus = source.at("conBonus").get<int>();
	statBoost.intBonus = source.at("intBonus").get<int>();
	statBoost.wisBonus = source.at("wisBonus").get<int>();
	statBoost.chaBonus = source.at("chaBonus").get<int>();
	statBoost.isSetMode = source.at("isSetMode").get<bool>();
	statBoost.exceptionalStrength = source.at("exceptionalStrength").get<int>();
}

// Shared use() for stat-boost equipment (Gauntlets, Girdle, JewelryAmulet): puts the item
// on, or takes it off. What it does to an ability is read from what is worn
// (Creature::calculate_effective_stat), never written here. An item that could not be put
// on or taken off - a cursed one that will not come off - uses no turn.
//
// Example:
//   use_stat_boost(EquipmentSlot::GIRDLE, girdle, player, ctx);   // -> true, worn: Strength 19
//   use_stat_boost(EquipmentSlot::GIRDLE, girdle, player, ctx);   // -> true, removed: Strength as before
bool use_stat_boost(EquipmentSlot slot, Item& item, Creature& wearer, GameContext& ctx)
{
	const bool wasEquipped = wearer.is_item_equipped(item.uniqueId);
	if (!wearer.toggle_equipment(item.uniqueId, slot, ctx))
	{
		return false;
	}

	wearer.update_armor_class(ctx);
	ctx.messageSystem->message(
		ColorPairId::WHITE_BLACK,
		std::format("You {} the {}.", wasEquipped ? "remove" : "put on", item.actorData.name),
		MessageCompletion::FINISHED);
	return true;
}

// Shared use() for magical equipment (MagicalHelm, MagicalRing)
bool use_magical_equip(MagicalEffect effect, EquipmentSlot slot, Item& item, Creature& wearer, GameContext& ctx)
{
	const bool wasEquipped = wearer.is_item_equipped(item.uniqueId);
	EquipmentSlot targetSlot = slot;

	// Rings: auto-choose open slot
	if (slot == EquipmentSlot::RIGHT_RING && !wasEquipped)
	{
		if (wearer.is_slot_occupied(EquipmentSlot::RIGHT_RING))
		{
			targetSlot = EquipmentSlot::LEFT_RING;
		}
	}

	const bool success = wearer.toggle_equipment(item.uniqueId, targetSlot, ctx);
	if (success)
	{
		if (wasEquipped)
		{
			if (effect == MagicalEffect::INVISIBILITY && wearer.is_invisible())
			{
				ctx.buffSystem->remove_buff(wearer, BuffType::INVISIBILITY);
				ctx.messageSystem->message(ColorPairId::CYAN_BLACK, "Your invisibility fades.", MessageCompletion::FINISHED);
			}
			ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "You remove the " + item.actorData.name + ".", MessageCompletion::FINISHED);
		}
		else
		{
			if (effect == MagicalEffect::INVISIBILITY)
			{
				ctx.messageSystem->message(ColorPairId::CYAN_BLACK, "The ring pulses with arcane power. Press Ctrl+C to cast.", MessageCompletion::FINISHED);
			}
			ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "You put on the " + item.actorData.name + ".", MessageCompletion::FINISHED);
		}

		if (MagicalEffectUtils::is_protection_effect(effect) || effect == MagicalEffect::BRILLIANCE)
		{
			wearer.update_armor_class(ctx);
		}

		return true;
	}

	// Could not be put on or taken off - a cursed item that will not come off: no turn.
	return false;
}

} // namespace

// ========== Weapon struct methods ==========

bool Weapon::validate_dual_wield(const Item* mainHand, const Item* offHand) const
{
	if (!mainHand || !offHand)
	{
		return false;
	}
	return mainHand->is_weapon() && offHand->is_weapon();
}

EquipmentSlot Weapon::get_preferred_slot(const Creature* wearer) const
{
	// Reached below for what is already in each hand. The two early returns mean a
	// null wearer answers correctly for a ranged or two-handed weapon and reaches
	// through the null for anything else, which is the intermittent kind of fault.
	assert(wearer && "get_preferred_slot: asked which hand a weapon prefers, with nobody to wear it");

	if (ranged)
	{
		return EquipmentSlot::MISSILE_WEAPON;
	}

	if (!can_be_off_hand())
	{
		return EquipmentSlot::RIGHT_HAND;
	}

	Item* mainHand = wearer->get_equipped_item(EquipmentSlot::RIGHT_HAND);
	Item* offHand = wearer->get_equipped_item(EquipmentSlot::LEFT_HAND);

	if (!mainHand || offHand)
	{
		return EquipmentSlot::RIGHT_HAND;
	}

	if (mainHand->is_weapon())
	{
		if (const Weapon* mainWeapon = std::get_if<Weapon>(&*mainHand->behavior))
		{
			if (mainWeapon->get_weapon_size() > weaponSize)
			{
				return EquipmentSlot::LEFT_HAND;
			}
		}
	}

	return EquipmentSlot::RIGHT_HAND;
}

// ========== use() implementations ==========

bool use(Consumable& consumable, Item& owner, Creature& wearer, GameContext& ctx)
{
	switch (consumable.effect)
	{

	case ConsumableEffect::HEAL:
	{
		if (wearer.get_hp() >= wearer.get_max_hp())
		{
			ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "You are already at full health.", MessageCompletion::FINISHED);
			return false;
		}
		const int healed = wearer.heal(consumable.amount);
		ctx.messageSystem->message(ColorPairId::GREEN_BLACK, std::format("You feel better! (+{} HP)", healed), MessageCompletion::FINISHED);
		break;
	}

	case ConsumableEffect::ADD_BUFF:
	{
		ctx.buffSystem->add_buff(wearer, consumable.buffType, consumable.amount, consumable.duration, consumable.isSetEffect);
		ctx.messageSystem->message(
			ColorPairId::CYAN_BLACK,
			std::format("You feel the effect of the {} for {} turns.", owner.get_name(), consumable.duration),
			MessageCompletion::FINISHED);
		break;
	}

	case ConsumableEffect::NONE:
	{
		ctx.messageSystem->message(ColorPairId::WHITE_BLACK, std::format("You use the {}.", owner.get_name()), MessageCompletion::FINISHED);
		break;
	}

	case ConsumableEffect::FAIL:
	{
		ctx.messageSystem->message(ColorPairId::RED_BLACK, std::format("Nothing happens with the {}.", owner.get_name()), MessageCompletion::FINISHED);
		return false;
	}
	}

	return consume_item(owner, wearer);
}

int strength_rating_of(const Item& item)
{
	// Only a weapon is made for an arm.
	const Weapon* weapon = item.behavior ? std::get_if<Weapon>(&*item.behavior) : nullptr;
	return weapon ? weapon->strengthRating : 0;
}

bool can_draw(const Creature& wielder, const Item& weapon)
{
	return wielder.get_strength() >= strength_rating_of(weapon);
}

bool use(Weapon& weapon, Item& owner, Creature& wearer, GameContext& ctx)
{
	const EquipmentSlot preferred = weapon.get_preferred_slot(&wearer);
	const bool success = wearer.toggle_weapon(owner.uniqueId, preferred, ctx);

	if (success)
	{
		Item* equipped = wearer.get_equipped_item(preferred);
		if (equipped && equipped->uniqueId == owner.uniqueId)
		{
			const std::string slotName = (preferred == EquipmentSlot::LEFT_HAND) ? "off-hand" : "main hand";
			ctx.messageSystem->message(
				ColorPairId::WHITE_BLACK,
				std::format("You equip the {} in your {}.", owner.get_name(), slotName),
				MessageCompletion::FINISHED);
		}
		else
		{
			ctx.messageSystem->message(ColorPairId::WHITE_BLACK, std::format("You unequip the {}.", owner.get_name()), MessageCompletion::FINISHED);
		}
		return true;
	}

	// Could not be put on or taken off - a cursed item that will not come off: no turn.
	return false;
}

bool use(TargetedScroll& targetScroll, Item& owner, Creature& wearer, GameContext& ctx)
{
	// FOV_BUFF and AUTO_NEAREST are synchronous — no targeting cursor needed
	if (targetScroll.targetMode == TargetMode::FOV_BUFF)
	{
		int affected = 0;
		for (const auto& creature : *ctx.creatures)
		{
			assert(creature);
			if (creature->is_dead())
			{
				continue;
			}

			if (!ctx.map->is_in_fov(creature->position))
			{
				continue;
			}

			// A scroll's effect is a spell: the target saves against it or takes it.
			if (!SavingThrows::is_made(*creature, SavingThrow::SPELL, 0, ctx))
			{
				ctx.buffSystem->add_buff(*creature, targetScroll.buffType, 0, targetScroll.buffDuration, false);
				++affected;
			}
		}

		if (affected > 0)
		{
			ctx.messageSystem->append_message_part(ColorPairId::CYAN_BLACK, std::format("{}! ", owner.get_name()));
			ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, std::format("{} creatures are affected.", affected));
			ctx.messageSystem->finalize_message();
		}
		else
		{
			ctx.messageSystem->message(ColorPairId::WHITE_BLACK, std::format("The {} has no effect.", owner.get_name()), MessageCompletion::FINISHED);
		}

		return consume_item(owner, wearer);
	}

	if (targetScroll.targetMode == TargetMode::AUTO_NEAREST)
	{
		TargetResult result = ctx.targeting->acquire_nearest(
			ctx,
			wearer,
			targetScroll.range);
		if (!result.success || result.creatures.empty())
		{
			return false;
		}
		auto* target = result.creatures[0];
		ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, "A lightning bolt strikes the ");
		ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLUE, target->actorData.name);
		ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, " with a loud thunder!");
		ctx.messageSystem->finalize_message();
		SpellAnimations::animate_lightning(wearer.position, target->position, ctx);
		ctx.messageSystem->message(ColorPairId::WHITE_RED, std::format("The damage is {} hit points.", targetScroll.damage), MessageCompletion::FINISHED);
		target->take_damage_and_check_death(targetScroll.damage, ctx, DamageType::LIGHTNING);
		ctx.creatureManager->cleanup_dead_creatures(*ctx.creatures);
		return consume_item(owner, wearer);
	}

	// PICK_TILE_SINGLE and PICK_TILE_AOE — async via TargetingMenu
	int aoeRadius = (targetScroll.targetMode == TargetMode::PICK_TILE_AOE) ? targetScroll.range : 0;
	int scrollRange = targetScroll.range;
	int scrollDamage = targetScroll.damage;
	int confuseTurns = targetScroll.confuseTurns;
	TargetMode mode = targetScroll.targetMode;

	auto onTarget = [mode, aoeRadius, scrollRange, scrollDamage, confuseTurns, &owner, &wearer](
						bool confirmed,
						Vector2D targetPos,
						GameContext& innerCtx) mutable
	{
		if (!confirmed)
		{
			return;
		}

		if (mode == TargetMode::PICK_TILE_AOE)
		{
			innerCtx.messageSystem->append_message_part(
				ColorPairId::WHITE_BLACK,
				std::format("The fireball explodes, burning everything within {} tiles!", scrollRange));
			innerCtx.messageSystem->finalize_message();
			// The scroll casts the spell itself, at the level the book reads it at.
			const SpellSystem::FireballBurst burst = SpellSystem::burst_fireball(
				targetPos, SpellSystem::SCROLL_FIREBALL_CASTER_LEVEL, aoeRadius, innerCtx);
			innerCtx.messageSystem->message(
				ColorPairId::WHITE_BLACK,
				std::format("{}d6 = {} fire, {} struck.", burst.diceCount, burst.totalDamage, burst.struck),
				MessageCompletion::FINISHED);

			if (innerCtx.player()->get_tile_distance(targetPos) <= aoeRadius)
			{
				SpellAnimations::animate_creature_hit(innerCtx.player()->position, innerCtx);
				SpellSystem::burn_with_fireball(*innerCtx.player(), burst, innerCtx);
			}
			innerCtx.creatureManager->cleanup_dead_creatures(*innerCtx.creatures);
		}
		else // PICK_TILE_SINGLE
		{
			// A scroll the reader could not bring themselves to read costs the turn, not the scroll.
			if (read_confusion_at(wearer, targetPos, confuseTurns, innerCtx) == ScrollReading::KEPT)
			{
				innerCtx.gameState->set_game_status(GameStatus::NEW_TURN);
				return;
			}
		}

		consume_item(owner, wearer);
		innerCtx.gameState->set_game_status(GameStatus::NEW_TURN);
	};

	ctx.menus->push_back(std::make_unique<TargetingMenu>(scrollRange, aoeRadius, std::move(onTarget), ctx));
	return false; // turn and item consumption handled in callback
}

ScrollReading read_confusion_at(const Creature& reader, Vector2D tile, int turns, GameContext& ctx)
{
	Creature* target = ctx.map->get_actor(tile, ctx);

	// The reader ignores a creature whose Sanctuary turns them away, and reads at nothing.
	if (target && ctx.buffSystem->is_turned_away_by_sanctuary(reader, *target, ctx))
	{
		ctx.messageSystem->message(
			ColorPairId::WHITE_BLACK,
			std::format("You cannot bring yourself to read the scroll at the {}.", target->actorData.name),
			MessageCompletion::FINISHED);
		return ScrollReading::KEPT;
	}

	if (target)
	{
		target->apply_confusion(turns);
		ctx.messageSystem->message(
			ColorPairId::WHITE_BLACK,
			std::format("The eyes of the {} look vacant, as he starts to stumble around!", target->actorData.name),
			MessageCompletion::FINISHED);
	}
	return ScrollReading::SPENT;
}

bool use(Gold& gold, Item& owner, Creature& wearer, GameContext& ctx)
{
	wearer.adjust_gold(gold.amount);
	ctx.messageSystem->append_message_part(ColorPairId::YELLOW_BLACK, "You gained ");
	ctx.messageSystem->append_message_part(ColorPairId::YELLOW_BLACK, std::to_string(gold.amount));
	ctx.messageSystem->append_message_part(ColorPairId::YELLOW_BLACK, " gold.");
	ctx.messageSystem->finalize_message();
	return consume_item(owner, wearer);
}

bool use(Food& food, Item& owner, Creature& wearer, GameContext& ctx)
{
	ctx.hungerSystem->decrease_hunger(ctx, food.nutritionValue);
	ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, "You eat the ");
	ctx.messageSystem->append_message_part(ColorPairId::YELLOW_BLACK, owner.actorData.name);
	ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, ".");
	ctx.messageSystem->finalize_message();
	return consume_item(owner, wearer);
}

bool use(CorpseFood& corpseFood, Item& owner, Creature& wearer, GameContext& ctx)
{
	if (corpseFood.nutritionValue <= 0)
	{
		corpseFood.nutritionValue = get_or_default(corpseNutritionValues, owner.actorData.name, 50);
	}

	int actual = corpseFood.nutritionValue + ctx.dice->roll(-10, 10);
	actual = std::max(10, actual);

	ctx.hungerSystem->decrease_hunger(ctx, actual);

	const std::string& flavor = get_or_default(corpseFlavorText, owner.actorData.name, std::string{ "It tastes... questionable." });

	ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, "You eat the ");
	ctx.messageSystem->append_message_part(ColorPairId::RED_BLACK, owner.actorData.name);
	ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, ". " + flavor);
	ctx.messageSystem->finalize_message();
	return consume_item(owner, wearer);
}

bool use(Armor& armor, Item& item, Creature& wearer, GameContext& ctx)
{
	const bool was_equipped = wearer.is_item_equipped(item.uniqueId);
	const bool success = wearer.toggle_armor(item.uniqueId, ctx);

	if (success)
	{
		ctx.messageSystem->message(
			ColorPairId::WHITE_BLACK,
			was_equipped ? "You remove the " + item.actorData.name + "."
						 : "You put on the " + item.actorData.name + ".",
			MessageCompletion::FINISHED);
	}

	return success;
}

bool use(MagicalHelm& magicalHelm, Item& owner, Creature& wearer, GameContext& ctx)
{
	return use_magical_equip(magicalHelm.effect, EquipmentSlot::HEAD, owner, wearer, ctx);
}

bool use(MagicalRing& magicalRing, Item& owner, Creature& wearer, GameContext& ctx)
{
	return use_magical_equip(magicalRing.effect, EquipmentSlot::RIGHT_RING, owner, wearer, ctx);
}

bool use(JewelryAmulet& jewelryAmulet, Item& owner, Creature& wearer, GameContext& ctx)
{
	return use_stat_boost(EquipmentSlot::NECK, owner, wearer, ctx);
}

bool use(Gauntlets& gauntlets, Item& owner, Creature& wearer, GameContext& ctx)
{
	return use_stat_boost(EquipmentSlot::GAUNTLETS, owner, wearer, ctx);
}

bool use(Girdle& girdle, Item& owner, Creature& wearer, GameContext& ctx)
{
	return use_stat_boost(EquipmentSlot::GIRDLE, owner, wearer, ctx);
}

bool use(Shield& shield, Item& owner, Creature& wearer, GameContext& ctx)
{
	const bool success = wearer.toggle_shield(owner.uniqueId, ctx);
	if (success)
	{
		Item* equipped = wearer.get_equipped_item(EquipmentSlot::LEFT_HAND);
		if (equipped && equipped->uniqueId == owner.uniqueId)
		{
			ctx.messageSystem->message(ColorPairId::WHITE_BLACK, std::format("You raise the {}.", owner.get_name()), MessageCompletion::FINISHED);
		}
		else
		{
			ctx.messageSystem->message(ColorPairId::WHITE_BLACK, std::format("You lower the {}.", owner.get_name()), MessageCompletion::FINISHED);
		}
		return true;
	}

	// Could not be put on or taken off - a cursed item that will not come off: no turn.
	return false;
}

bool use(Teleporter& teleporter, Item& owner, Creature& wearer, GameContext& ctx)
{
	wearer.position = SpawnUtils::find_random_floor_tile(ctx);
	ctx.map->compute_fov(ctx);
	ctx.messageSystem->message(ColorPairId::BLUE_BLACK, "You feel disoriented as the world shifts around you!", MessageCompletion::FINISHED);
	ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "You have been teleported to a new location.", MessageCompletion::FINISHED);
	return consume_item(owner, wearer);
}

// Identifies every item in the wearer's pack that is not already fully identified, and
// says how many that was. The scroll is consumed either way: reading it is the cost, and
// a pack that was already identified still burns it.
//
// Example, a pack holding two unknown potions and one known sword:
//   use(scroll, scrollItem, player, ctx);   // -> true, "2 items identified!"
//   use(scroll, scrollItem, player, ctx);   // -> true, "All items were already identified."
bool use(IdentifyScroll& identifyScroll, Item& owner, Creature& wearer, GameContext& ctx)
{
	int identifiedCount = 0;
	for (auto& item : wearer.inventoryData.items)
	{
		assert(item && "identify scroll: the pack holds a null where an item should be");

		if (!item->is_fully_identified())
		{
			item->identify_all();
			++identifiedCount;
			if (ctx.floatingText)
			{
				ctx.floatingText->spawn_text(
					wearer.position,
					std::string(item->get_name()) + " identified!",
					0,
					255,
					255,
					2.0f);
			}
		}
	}
	ctx.messageSystem->message(
		ColorPairId::CYAN_BLACK,
		identifiedCount > 0
			? std::format("You use the {}. {} items identified!", owner.get_name(), identifiedCount)
			: std::format("You use the {}. All items were already identified.", owner.get_name()),
		MessageCompletion::FINISHED);
	return consume_item(owner, wearer);
}

bool use(Amulet& amulet, Item& owner, Creature& wearer, GameContext& ctx)
{
	ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "The Amulet of Yendor glows brightly in your hands!", MessageCompletion::FINISHED);
	ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "You feel a powerful magic enveloping you...", MessageCompletion::FINISHED);
	ctx.gameState->set_game_status(GameStatus::VICTORY);
	return false;
}

bool use(DungeonKey& key, Item& owner, Creature& wearer, GameContext& ctx)
{
	ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "Bump into a locked door to use this key.", MessageCompletion::FINISHED);
	return false;
}

// ========== Variant-level dispatchers ==========

bool use_item(ItemBehavior& behavior, Item& owner, Creature& wearer, GameContext& ctx)
{
	return std::visit(
		[&owner, &wearer, &ctx](auto& behavior) -> bool
		{
			return use(behavior, owner, wearer, ctx);
		},
		behavior);
}

int get_item_ac_bonus(const ItemBehavior& behavior) noexcept
{
	return std::visit(
		VariantVisitor{
			[](const Armor& a) -> int
			{ return a.armorClass; },
			[](const Shield&) -> int
			{ return -1; }, // +1 AC in AD&D terms
			[](const MagicalHelm& mh) -> int
			{ return MagicalEffectUtils::get_ac_bonus(mh.effect, mh.bonus); },
			[](const MagicalRing& mr) -> int
			{ return MagicalEffectUtils::get_protection_bonus(mr.effect); },
			[](const auto&) -> int
			{ return 0; },
		},
		behavior);
}

// The damage type a resistance effect answers for; empty for an effect that
// resists nothing.
static std::optional<DamageType> resisted_type(MagicalEffect effect) noexcept
{
	switch (effect)
	{
	case MagicalEffect::FIRE_RESISTANCE:
	case MagicalEffect::BRILLIANCE:
	{
		return DamageType::FIRE;
	}
	case MagicalEffect::COLD_RESISTANCE:
	{
		return DamageType::COLD;
	}
	default:
	{
		return std::nullopt;
	}
	}
}

int get_item_resistance_strength(const ItemBehavior& behavior, DamageType damageType) noexcept
{
	// A ring or helm resists one type at its bonus; nothing else worn resists at all.
	const auto percent_if = [damageType](MagicalEffect effect, int bonus) -> int
	{
		const std::optional<DamageType> resisted = resisted_type(effect);
		return resisted.has_value() && *resisted == damageType ? bonus : 0;
	};
	return std::visit(
		VariantVisitor{
			[&percent_if](const MagicalRing& ring) -> int
			{ return percent_if(ring.effect, ring.bonus); },
			[&percent_if](const MagicalHelm& helm) -> int
			{ return percent_if(helm.effect, helm.bonus); },
			[](const auto&) -> int
			{ return 0; },
		},
		behavior);
}

// ========== Serialization ==========

void save_behavior(const ItemBehavior& behavior, json& output)
{
	std::visit(
		VariantVisitor{
			[&output](const Consumable& b)
			{
				output["type"] = encode_pickable_type(PickableType::CONSUMABLE);
				output["effect"] = encode_consumable_effect(b.effect);
				output["amount"] = b.amount;
				output["duration"] = b.duration;
				output["buffType"] = encode_buff_type(b.buffType);
				output["isSetEffect"] = b.isSetEffect;
			},
			[&output](const Weapon& b)
			{
				output["type"] = encode_pickable_type(PickableType::WEAPON);
				output["ranged"] = b.ranged;
				output["handRequirement"] = encode_hand_requirement(b.handRequirement);
				output["weaponSize"] = encode_weapon_size(b.weaponSize);
				output["strengthRating"] = b.strengthRating;
			},
			[&output](const Shield&)
			{ output["type"] = encode_pickable_type(PickableType::SHIELD); },
			[&output](const TargetedScroll& b)
			{
				output["type"] = encode_pickable_type(PickableType::TARGETED_SCROLL);
				output["targetMode"] = encode_target_mode(b.targetMode);
				output["scrollAnimation"] = encode_scroll_animation(b.scrollAnimation);
				output["range"] = b.range;
				output["damage"] = b.damage;
				output["confuseTurns"] = b.confuseTurns;
				output["buffType"] = encode_buff_type(b.buffType);
				output["buffDuration"] = b.buffDuration;
			},
			[&output](const Teleporter&)
			{ output["type"] = encode_pickable_type(PickableType::TELEPORTER); },
			[&output](const IdentifyScroll&)
			{ output["type"] = encode_pickable_type(PickableType::IDENTIFY_SCROLL); },
			[&output](const Gold& b)
			{
				output["type"] = encode_pickable_type(PickableType::GOLD_COIN);
				output["amount"] = b.amount;
			},
			[&output](const Food& b)
			{
				output["type"] = encode_pickable_type(PickableType::FOOD);
				output["nutritionValue"] = b.nutritionValue;
			},
			[&output](const CorpseFood& b)
			{
				output["type"] = encode_pickable_type(PickableType::CORPSE_FOOD);
				output["nutritionValue"] = b.nutritionValue;
			},
			[&output](const Armor& b)
			{
				output["type"] = encode_pickable_type(PickableType::ARMOR);
				output["armorClass"] = b.armorClass;
			},
			[&output](const MagicalHelm& b)
			{
				output["type"] = encode_pickable_type(PickableType::MAGICAL_HELM);
				output["effect"] = encode_magical_effect(b.effect);
				output["bonus"] = b.bonus;
			},
			[&output](const MagicalRing& b)
			{
				output["type"] = encode_pickable_type(PickableType::MAGICAL_RING);
				output["effect"] = encode_magical_effect(b.effect);
				output["bonus"] = b.bonus;
			},
			[&output](const JewelryAmulet& b)
			{ save_stat_boost(b, PickableType::JEWELRY_AMULET, output); },
			[&output](const Gauntlets& b)
			{ save_stat_boost(b, PickableType::GAUNTLETS, output); },
			[&output](const Girdle& b)
			{ save_stat_boost(b, PickableType::GIRDLE, output); },
			[&output](const Amulet&)
			{ output["type"] = encode_pickable_type(PickableType::QUEST_ITEM); },
			[&output](const DungeonKey&)
			{ output["type"] = encode_pickable_type(PickableType::DUNGEON_KEY); },
		},
		behavior);
}

ItemBehavior load_behavior(const json& source)
{
	const PickableType type = parse_pickable_type(source.at("type").get<std::string>());

	switch (type)
	{

	case PickableType::CONSUMABLE:
	{
		Consumable consumable;
		consumable.effect = parse_consumable_effect(source.at("effect").get<std::string>());
		consumable.amount = source.at("amount").get<int>();
		consumable.duration = source.at("duration").get<int>();
		consumable.buffType = parse_buff_type(source.at("buffType").get<std::string>());
		consumable.isSetEffect = source.at("isSetEffect").get<bool>();
		return consumable;
	}

	case PickableType::WEAPON:
	{
		Weapon weapon;
		weapon.ranged = source.at("ranged").get<bool>();
		weapon.handRequirement = parse_hand_requirement(source.at("handRequirement").get<std::string>());
		weapon.weaponSize = parse_weapon_size(source.at("weaponSize").get<std::string>());
		weapon.strengthRating = source.at("strengthRating").get<int>();
		return weapon;
	}

	case PickableType::SHIELD:
	{
		return Shield{};
	}

	case PickableType::TARGETED_SCROLL:
	{
		TargetedScroll targetedScroll;
		targetedScroll.targetMode = parse_target_mode(source.at("targetMode").get<std::string>());
		targetedScroll.scrollAnimation = parse_scroll_animation(source.at("scrollAnimation").get<std::string>());
		targetedScroll.range = source.at("range").get<int>();
		targetedScroll.damage = source.at("damage").get<int>();
		targetedScroll.confuseTurns = source.at("confuseTurns").get<int>();
		targetedScroll.buffType = parse_buff_type(source.at("buffType").get<std::string>());
		targetedScroll.buffDuration = source.at("buffDuration").get<int>();
		return targetedScroll;
	}

	case PickableType::TELEPORTER:
	{
		return Teleporter{};
	}

	case PickableType::IDENTIFY_SCROLL:
	{
		return IdentifyScroll{};
	}

	case PickableType::GOLD_COIN:
	{
		Gold gold;
		gold.amount = source.at("amount").get<int>();
		return gold;
	}

	case PickableType::FOOD:
	{
		Food food;
		food.nutritionValue = source.at("nutritionValue").get<int>();
		return food;
	}

	case PickableType::CORPSE_FOOD:
	{
		CorpseFood corpseFood;
		corpseFood.nutritionValue = source.at("nutritionValue").get<int>();
		return corpseFood;
	}

	case PickableType::ARMOR:
	{
		Armor armor;
		armor.armorClass = source.at("armorClass").get<int>();
		return armor;
	}

	case PickableType::MAGICAL_HELM:
	{
		MagicalHelm magicalHelm;
		magicalHelm.effect = parse_magical_effect(source.at("effect").get<std::string>());
		magicalHelm.bonus = source.at("bonus").get<int>();
		return magicalHelm;
	}

	case PickableType::MAGICAL_RING:
	{
		MagicalRing magicalRing;
		magicalRing.effect = parse_magical_effect(source.at("effect").get<std::string>());
		magicalRing.bonus = source.at("bonus").get<int>();
		return magicalRing;
	}

	case PickableType::JEWELRY_AMULET:
	{
		JewelryAmulet jewelryAmulet;
		load_stat_boost(jewelryAmulet, source);
		return jewelryAmulet;
	}

	case PickableType::GAUNTLETS:
	{
		Gauntlets gauntlets;
		load_stat_boost(gauntlets, source);
		return gauntlets;
	}

	case PickableType::GIRDLE:
	{
		Girdle girdle;
		load_stat_boost(girdle, source);
		return girdle;
	}

	case PickableType::QUEST_ITEM:
	{
		return Amulet{};
	}

	case PickableType::DUNGEON_KEY:
	{
		return DungeonKey{};
	}

	default:
	{
		throw std::runtime_error(std::format("no loader for behaviour '{}'", encode_pickable_type(type)));
	}
	}
}
