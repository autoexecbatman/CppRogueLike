#include <algorithm>
#include <cassert>
#include <format>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>

#include "../Utils/VariantVisitor.h"

#include "../Actor/Creature.h"
#include "../ActorTypes/Player.h"
#include "../Colors/Colors.h"
#include "../Core/GameContext.h"
#include "../Items/MagicalItemEffects.h"
#include "../Items/Weapons.h"
#include "../Map/Map.h"
#include "../Persistent/Persistent.h"
#include "../Systems/BuffSystem.h"
#include "../Systems/BuffType.h"
#include "../Systems/CreatureManager.h"
#include "../Systems/FloatingTextSystem.h"
#include "../Systems/HungerSystem.h"
#include "../Systems/MessageSystem.h"
#include "../Systems/SpawnUtils.h"
#include "../Systems/SpellAnimations.h"
#include "../Systems/TargetingMenu.h"
#include "../Systems/TargetingSystem.h"
#include "../Systems/TargetMode.h"
#include "../Utils/Vector2D.h"
#include "Actor.h"
#include "EquipmentSlot.h"
#include "InventoryData.h"
#include "InventoryOperations.h"
#include "Pickable.h"

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
	output["type"] = static_cast<int>(type);
	output["strBonus"] = statBoost.strBonus;
	output["dexBonus"] = statBoost.dexBonus;
	output["conBonus"] = statBoost.conBonus;
	output["intBonus"] = statBoost.intBonus;
	output["wisBonus"] = statBoost.wisBonus;
	output["chaBonus"] = statBoost.chaBonus;
	output["isSetMode"] = statBoost.isSetMode;
	output["originalStats"] = {
		{ "str", statBoost.originalStats.str },
		{ "dex", statBoost.originalStats.dex },
		{ "con", statBoost.originalStats.con },
		{ "intel", statBoost.originalStats.intel },
		{ "wis", statBoost.originalStats.wis },
		{ "cha", statBoost.originalStats.cha },
	};
}

template <typename T>
void load_stat_boost(T& statBoost, const json& source)
{
	statBoost.strBonus = source.contains("strBonus") ? source.at("strBonus").get<int>() : 0;
	statBoost.dexBonus = source.contains("dexBonus") ? source.at("dexBonus").get<int>() : 0;
	statBoost.conBonus = source.contains("conBonus") ? source.at("conBonus").get<int>() : 0;
	statBoost.intBonus = source.contains("intBonus") ? source.at("intBonus").get<int>() : 0;
	statBoost.wisBonus = source.contains("wisBonus") ? source.at("wisBonus").get<int>() : 0;
	statBoost.chaBonus = source.contains("chaBonus") ? source.at("chaBonus").get<int>() : 0;
	statBoost.isSetMode = source.contains("isSetMode") ? source.at("isSetMode").get<bool>() : false;

	if (source.contains("originalStats"))
	{
		const auto& orig = source.at("originalStats");
		statBoost.originalStats.str = orig.contains("str") ? orig.at("str").get<int>() : 0;
		statBoost.originalStats.dex = orig.contains("dex") ? orig.at("dex").get<int>() : 0;
		statBoost.originalStats.con = orig.contains("con") ? orig.at("con").get<int>() : 0;
		statBoost.originalStats.intel = orig.contains("intel") ? orig.at("intel").get<int>() : 0;
		statBoost.originalStats.wis = orig.contains("wis") ? orig.at("wis").get<int>() : 0;
		statBoost.originalStats.cha = orig.contains("cha") ? orig.at("cha").get<int>() : 0;
	}
}

// Shared use() for stat-boost equipment (Gauntlets, Girdle, JewelryAmulet)
template <typename T>
bool use_stat_boost(T& statBoost, EquipmentSlot slot, Item& item, Player& wearer, GameContext& ctx)
{
	auto apply_stat_boost = [&]()
	{
		if (statBoost.isSetMode)
		{
			if (statBoost.strBonus != 0) { statBoost.originalStats.str = wearer.get_strength(); wearer.set_strength(statBoost.strBonus); }
			if (statBoost.dexBonus != 0) { statBoost.originalStats.dex = wearer.get_dexterity(); wearer.set_dexterity(statBoost.dexBonus); }
			if (statBoost.conBonus != 0) { statBoost.originalStats.con = wearer.get_constitution(); wearer.set_constitution(statBoost.conBonus); }
			if (statBoost.intBonus != 0) { statBoost.originalStats.intel = wearer.get_intelligence(); wearer.set_intelligence(statBoost.intBonus); }
			if (statBoost.wisBonus != 0) { statBoost.originalStats.wis = wearer.get_wisdom(); wearer.set_wisdom(statBoost.wisBonus); }
			if (statBoost.chaBonus != 0) { statBoost.originalStats.cha = wearer.get_charisma(); wearer.set_charisma(statBoost.chaBonus); }
		}
		else
		{
			wearer.set_strength(wearer.get_strength() + statBoost.strBonus);
			wearer.set_dexterity(wearer.get_dexterity() + statBoost.dexBonus);
			wearer.set_constitution(wearer.get_constitution() + statBoost.conBonus);
			wearer.set_intelligence(wearer.get_intelligence() + statBoost.intBonus);
			wearer.set_wisdom(wearer.get_wisdom() + statBoost.wisBonus);
			wearer.set_charisma(wearer.get_charisma() + statBoost.chaBonus);
		}
	};

	auto remove_stat_boost = [&]()
	{
		if (statBoost.isSetMode)
		{
			if (statBoost.strBonus != 0) { wearer.set_strength(statBoost.originalStats.str); }
			if (statBoost.dexBonus != 0) { wearer.set_dexterity(statBoost.originalStats.dex); }
			if (statBoost.conBonus != 0) { wearer.set_constitution(statBoost.originalStats.con); }
			if (statBoost.intBonus != 0) { wearer.set_intelligence(statBoost.originalStats.intel); }
			if (statBoost.wisBonus != 0) { wearer.set_wisdom(statBoost.originalStats.wis); }
			if (statBoost.chaBonus != 0) { wearer.set_charisma(statBoost.originalStats.cha); }
		}
		else
		{
			wearer.set_strength(wearer.get_strength() - statBoost.strBonus);
			wearer.set_dexterity(wearer.get_dexterity() - statBoost.dexBonus);
			wearer.set_constitution(wearer.get_constitution() - statBoost.conBonus);
			wearer.set_intelligence(wearer.get_intelligence() - statBoost.intBonus);
			wearer.set_wisdom(wearer.get_wisdom() - statBoost.wisBonus);
			wearer.set_charisma(wearer.get_charisma() - statBoost.chaBonus);
		}
	};

	const bool wasEquipped = wearer.is_item_equipped(item.uniqueId);
	const bool success = wearer.toggle_equipment(item.uniqueId, slot, ctx);

	if (success)
	{
		wasEquipped ? remove_stat_boost() : apply_stat_boost();
		wearer.update_armor_class(ctx);
		ctx.messageSystem->message(
			WHITE_BLACK_PAIR,
			wasEquipped ? "You remove the " + item.actorData.name + "."
			            : "You put on the " + item.actorData.name + ".",
			true);
		return true;
	}

	// NPC fallback: toggle equipped state + apply bonuses directly
	if (item.has_state(ActorState::IS_EQUIPPED))
	{
		item.remove_state(ActorState::IS_EQUIPPED);
		remove_stat_boost();
	}
	else
	{
		item.add_state(ActorState::IS_EQUIPPED);
		apply_stat_boost();
	}
	wearer.update_armor_class(ctx);
	return true;
}

// Shared use() for magical equipment (MagicalHelm, MagicalRing)
bool use_magical_equip(MagicalEffect effect, EquipmentSlot slot, Item& item, Player& wearer, GameContext& ctx)
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
				ctx.messageSystem->message(CYAN_BLACK_PAIR, "Your invisibility fades.", true);
			}
			ctx.messageSystem->message(WHITE_BLACK_PAIR, "You remove the " + item.actorData.name + ".", true);
		}
		else
		{
			if (effect == MagicalEffect::INVISIBILITY)
			{
				ctx.messageSystem->message(CYAN_BLACK_PAIR, "The ring pulses with arcane power. Press Ctrl+C to cast.", true);
			}
			ctx.messageSystem->message(WHITE_BLACK_PAIR, "You put on the " + item.actorData.name + ".", true);
		}

		if (MagicalEffectUtils::is_protection_effect(effect) || effect == MagicalEffect::BRILLIANCE)
		{
			wearer.update_armor_class(ctx);
		}

		return true;
	}

	// NPC fallback: toggle equipped state
	if (!item.has_state(ActorState::IS_EQUIPPED))
	{
		item.add_state(ActorState::IS_EQUIPPED);
	}
	else
	{
		item.remove_state(ActorState::IS_EQUIPPED);
	}

	return true;
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

EquipmentSlot Weapon::get_preferred_slot(const Player* player) const
{
	if (ranged)
	{
		return EquipmentSlot::MISSILE_WEAPON;
	}

	if (!can_be_off_hand())
	{
		return EquipmentSlot::RIGHT_HAND;
	}

	Item* mainHand = player->get_equipped_item(EquipmentSlot::RIGHT_HAND);
	Item* offHand = player->get_equipped_item(EquipmentSlot::LEFT_HAND);

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
			ctx.messageSystem->message(WHITE_BLACK_PAIR, "You are already at full health.", true);
			return false;
		}
		const int healed = wearer.heal(consumable.amount);
		ctx.messageSystem->message(GREEN_BLACK_PAIR, std::format("You feel better! (+{} HP)", healed), true);
		break;
	}

	case ConsumableEffect::ADD_BUFF:
	{
		ctx.buffSystem->add_buff(wearer, consumable.buffType, consumable.amount, consumable.duration, consumable.isSetEffect);
		ctx.messageSystem->message(
			CYAN_BLACK_PAIR,
			std::format("You feel the effect of the {} for {} turns.", owner.get_name(), consumable.duration),
			true);
		break;
	}

	case ConsumableEffect::NONE:
	{
		ctx.messageSystem->message(WHITE_BLACK_PAIR, std::format("You use the {}.", owner.get_name()), true);
		break;
	}

	case ConsumableEffect::FAIL:
	{
		ctx.messageSystem->message(RED_BLACK_PAIR, std::format("Nothing happens with the {}.", owner.get_name()), true);
		return false;
	}

	}

	return consume_item(owner, wearer);
}

bool use(Weapon& weapon, Item& owner, Player& wearer, GameContext& ctx)
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
				WHITE_BLACK_PAIR,
				std::format("You equip the {} in your {}.", owner.get_name(), slotName),
				true);
		}
		else
		{
			ctx.messageSystem->message(WHITE_BLACK_PAIR, std::format("You unequip the {}.", owner.get_name()), true);
		}
		return true;
	}

	wearer.equip(owner, ctx);
	return true;
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

			const int save = ctx.dice->roll(1, 20);
			if (save < 15)
			{
				ctx.buffSystem->add_buff(*creature, targetScroll.buffType, 0, targetScroll.buffDuration, false);
				++affected;
			}
		}

		if (affected > 0)
		{
			ctx.messageSystem->append_message_part(CYAN_BLACK_PAIR, std::format("{}! ", owner.get_name()));
			ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, std::format("{} creatures are affected.", affected));
			ctx.messageSystem->finalize_message();
		}
		else
		{
			ctx.messageSystem->message(WHITE_BLACK_PAIR, std::format("The {} has no effect.", owner.get_name()), true);
		}

		return consume_item(owner, wearer);
	}

	if (targetScroll.targetMode == TargetMode::AUTO_NEAREST)
	{
		TargetResult result = ctx.targeting->acquire_nearest(
			ctx,
			wearer.position,
			targetScroll.range);
		if (!result.success || result.creatures.empty())
		{
			return false;
		}
		auto* target = result.creatures[0];
		ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, "A lightning bolt strikes the ");
		ctx.messageSystem->append_message_part(WHITE_BLUE_PAIR, target->actorData.name);
		ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, " with a loud thunder!");
		ctx.messageSystem->finalize_message();
		SpellAnimations::animate_lightning(wearer.position, target->position, ctx);
		ctx.messageSystem->message(WHITE_RED_PAIR, std::format("The damage is {} hit points.", targetScroll.damage), true);
		target->take_damage_and_check_death(targetScroll.damage, ctx);
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
				WHITE_BLACK_PAIR,
				std::format("The fireball explodes, burning everything within {} tiles!", scrollRange));
			innerCtx.messageSystem->finalize_message();
			SpellAnimations::animate_explosion(targetPos, aoeRadius, innerCtx);

			if (innerCtx.player()->get_tile_distance(targetPos) <= aoeRadius)
			{
				SpellAnimations::animate_creature_hit(innerCtx.player()->position, innerCtx);
				innerCtx.player()->take_damage_and_check_death(scrollDamage, innerCtx);
			}

			for (const auto& creature : *innerCtx.creatures)
			{
				assert(creature);
				if (creature->is_dead())
				{
					continue;
				}
				if (creature->get_tile_distance(targetPos) > aoeRadius)
				{
					continue;
				}
				SpellAnimations::animate_creature_hit(creature->position, innerCtx);
				innerCtx.messageSystem->append_message_part(
					WHITE_BLACK_PAIR,
					std::format("The {} gets engulfed in flames! ({} damage)", creature->actorData.name, scrollDamage));
				innerCtx.messageSystem->finalize_message();
				creature->take_damage_and_check_death(scrollDamage, innerCtx);
			}
			innerCtx.creatureManager->cleanup_dead_creatures(*innerCtx.creatures);
		}
		else // PICK_TILE_SINGLE
		{
			Creature* target = innerCtx.map->get_actor(targetPos, innerCtx);
			if (target)
			{
				target->apply_confusion(confuseTurns);
				innerCtx.messageSystem->message(
					WHITE_BLACK_PAIR,
					std::format("The eyes of the {} look vacant, as he starts to stumble around!", target->actorData.name),
					true);
			}
		}

		consume_item(owner, wearer);
		innerCtx.gameState->set_game_status(GameStatus::NEW_TURN);
	};

	ctx.menus->push_back(std::make_unique<TargetingMenu>(scrollRange, aoeRadius, std::move(onTarget), ctx));
	return false; // turn and item consumption handled in callback
}

bool use(Gold& gold, Item& owner, Creature& wearer, GameContext& ctx)
{
	wearer.adjust_gold(gold.amount);
	ctx.messageSystem->append_message_part(YELLOW_BLACK_PAIR, "You gained ");
	ctx.messageSystem->append_message_part(YELLOW_BLACK_PAIR, std::to_string(gold.amount));
	ctx.messageSystem->append_message_part(YELLOW_BLACK_PAIR, " gold.");
	ctx.messageSystem->finalize_message();
	return consume_item(owner, wearer);
}

bool use(Food& food, Item& owner, Creature& wearer, GameContext& ctx)
{
	ctx.hungerSystem->decrease_hunger(ctx, food.nutritionValue);
	ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, "You eat the ");
	ctx.messageSystem->append_message_part(YELLOW_BLACK_PAIR, owner.actorData.name);
	ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, ".");
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

	ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, "You eat the ");
	ctx.messageSystem->append_message_part(RED_BLACK_PAIR, owner.actorData.name);
	ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, ". " + flavor);
	ctx.messageSystem->finalize_message();
	return consume_item(owner, wearer);
}

bool use(Armor& armor, Item& item, Player& wearer, GameContext& ctx)
{
	const bool was_equipped = wearer.is_item_equipped(item.uniqueId);
	const bool success = wearer.toggle_armor(item.uniqueId, ctx);

	if (success)
	{
		ctx.messageSystem->message(
			WHITE_BLACK_PAIR,
			was_equipped ? "You remove the " + item.actorData.name + "."
			             : "You put on the " + item.actorData.name + ".",
			true);
	}

	return success;
}

bool use(MagicalHelm& magicalHelm, Item& owner, Player& wearer, GameContext& ctx)
{
	return use_magical_equip(magicalHelm.effect, EquipmentSlot::HEAD, owner, wearer, ctx);
}

bool use(MagicalRing& magicalRing, Item& owner, Player& wearer, GameContext& ctx)
{
	return use_magical_equip(magicalRing.effect, EquipmentSlot::RIGHT_RING, owner, wearer, ctx);
}

bool use(JewelryAmulet& jewelryAmulet, Item& owner, Player& wearer, GameContext& ctx)
{
	return use_stat_boost(jewelryAmulet, EquipmentSlot::NECK, owner, wearer, ctx);
}

bool use(Gauntlets& gauntlets, Item& owner, Player& wearer, GameContext& ctx)
{
	return use_stat_boost(gauntlets, EquipmentSlot::GAUNTLETS, owner, wearer, ctx);
}

bool use(Girdle& girdle, Item& owner, Player& wearer, GameContext& ctx)
{
	return use_stat_boost(girdle, EquipmentSlot::GIRDLE, owner, wearer, ctx);
}

bool use(Shield& shield, Item& owner, Player& wearer, GameContext& ctx)
{
	const bool success = wearer.toggle_shield(owner.uniqueId, ctx);
	if (success)
	{
		Item* equipped = wearer.get_equipped_item(EquipmentSlot::LEFT_HAND);
		if (equipped && equipped->uniqueId == owner.uniqueId)
		{
			ctx.messageSystem->message(WHITE_BLACK_PAIR, std::format("You raise the {}.", owner.get_name()), true);
		}
		else
		{
			ctx.messageSystem->message(WHITE_BLACK_PAIR, std::format("You lower the {}.", owner.get_name()), true);
		}
		return true;
	}
	wearer.equip(owner, ctx);
	return true;
}

bool use(Teleporter& teleporter, Item& owner, Creature& wearer, GameContext& ctx)
{
	wearer.position = SpawnUtils::find_random_floor_tile(ctx);
	ctx.map->compute_fov(ctx);
	ctx.messageSystem->message(BLUE_BLACK_PAIR, "You feel disoriented as the world shifts around you!", true);
	ctx.messageSystem->message(WHITE_BLACK_PAIR, "You have been teleported to a new location.", true);
	return consume_item(owner, wearer);
}

bool use(IdentifyScroll& identifyScroll, Item& owner, Creature& wearer, GameContext& ctx)
{
	int identifiedCount = 0;
	for (auto& item : wearer.inventoryData.items)
	{
		if (!item->is_fully_identified())
		{
			item->identify_all();
			++identifiedCount;
			if (ctx.floatingText)
			{
				ctx.floatingText->spawn_text(
					wearer.position,
					std::string(item->get_name()) + " identified!",
					0, 255, 255, 2.0f);
			}
		}
	}
	ctx.messageSystem->message(
		CYAN_BLACK_PAIR,
		identifiedCount > 0
			? std::format("You use the {}. {} items identified!", owner.get_name(), identifiedCount)
			: std::format("You use the {}. All items were already identified.", owner.get_name()),
		true);
	return consume_item(owner, wearer);
}

bool use(Amulet& amulet, Item& owner, Creature& wearer, GameContext& ctx)
{
	ctx.messageSystem->message(WHITE_BLACK_PAIR, "The Amulet of Yendor glows brightly in your hands!", true);
	ctx.messageSystem->message(WHITE_BLACK_PAIR, "You feel a powerful magic enveloping you...", true);
	ctx.gameState->set_game_status(GameStatus::VICTORY);
	return false;
}

bool use(DungeonKey& key, Item& owner, Creature& wearer, GameContext& ctx)
{
	ctx.messageSystem->message(WHITE_BLACK_PAIR, "Bump into a locked door to use this key.", true);
	return false;
}

// ========== Variant-level dispatchers ==========

bool use_item(ItemBehavior& behavior, Item& owner, Player& wearer, GameContext& ctx)
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
			[](const Armor& a) -> int { return a.armorClass; },
			[](const Shield&) -> int { return -1; }, // +1 AC in AD&D terms
			[](const MagicalHelm& mh) -> int { return MagicalEffectUtils::get_ac_bonus(mh.effect, mh.bonus); },
			[](const MagicalRing& mr) -> int { return MagicalEffectUtils::get_protection_bonus(mr.effect); },
			[](const auto&) -> int { return 0; },
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
				output["type"] = static_cast<int>(PickableType::CONSUMABLE);
				output["effect"] = static_cast<int>(b.effect);
				output["amount"] = b.amount;
				output["duration"] = b.duration;
				output["buffType"] = static_cast<int>(b.buffType);
				output["isSetEffect"] = b.isSetEffect;
			},
			[&output](const Weapon& b)
			{
				output["type"] = static_cast<int>(PickableType::WEAPON);
				output["ranged"] = b.ranged;
				output["handReq"] = static_cast<int>(b.handRequirement);
				output["weaponSize"] = static_cast<int>(b.weaponSize);
			},
			[&output](const Shield&) { output["type"] = static_cast<int>(PickableType::SHIELD); },
			[&output](const TargetedScroll& b)
			{
				output["type"] = static_cast<int>(PickableType::TARGETED_SCROLL);
				output["targetMode"] = static_cast<int>(b.targetMode);
				output["scrollAnimation"] = static_cast<int>(b.scrollAnimation);
				output["range"] = b.range;
				output["damage"] = b.damage;
				output["confuseTurns"] = b.confuseTurns;
				output["buffType"] = static_cast<int>(b.buffType);
				output["buffDuration"] = b.buffDuration;
			},
			[&output](const Teleporter&) { output["type"] = static_cast<int>(PickableType::TELEPORTER); },
			[&output](const IdentifyScroll&) { output["type"] = static_cast<int>(PickableType::IDENTIFY_SCROLL); },
			[&output](const Gold& b)
			{
				output["type"] = static_cast<int>(PickableType::GOLD_COIN);
				output["amount"] = b.amount;
			},
			[&output](const Food& b)
			{
				output["type"] = static_cast<int>(PickableType::FOOD);
				output["nutritionValue"] = b.nutritionValue;
			},
			[&output](const CorpseFood& b)
			{
				output["type"] = static_cast<int>(PickableType::CORPSE_FOOD);
				output["nutritionValue"] = b.nutritionValue;
			},
			[&output](const Armor& b)
			{
				output["type"] = static_cast<int>(PickableType::ARMOR);
				output["armorClass"] = b.armorClass;
			},
			[&output](const MagicalHelm& b)
			{
				output["type"] = static_cast<int>(PickableType::MAGICAL_HELM);
				output["effect"] = static_cast<int>(b.effect);
				output["bonus"] = b.bonus;
			},
			[&output](const MagicalRing& b)
			{
				output["type"] = static_cast<int>(PickableType::MAGICAL_RING);
				output["effect"] = static_cast<int>(b.effect);
				output["bonus"] = b.bonus;
			},
			[&output](const JewelryAmulet& b) { save_stat_boost(b, PickableType::JEWELRY_AMULET, output); },
			[&output](const Gauntlets& b) { save_stat_boost(b, PickableType::GAUNTLETS, output); },
			[&output](const Girdle& b) { save_stat_boost(b, PickableType::GIRDLE, output); },
			[&output](const Amulet&) { output["type"] = static_cast<int>(PickableType::QUEST_ITEM); },
			[&output](const DungeonKey&) { output["type"] = static_cast<int>(PickableType::DUNGEON_KEY); },
		},
		behavior);
}

ItemBehavior load_behavior(const json& source)
{
	if (!source.contains("type") || !source["type"].is_number())
	{
		throw std::runtime_error("Invalid JSON format: Missing or invalid 'type'");
	}

	const auto type = static_cast<PickableType>(source["type"].get<int>());

	switch (type)
	{

	case PickableType::CONSUMABLE:
	{
		Consumable consumable;
		if (source.contains("effect"))
		{
			consumable.effect = static_cast<ConsumableEffect>(source["effect"].get<int>());
		}

		// Legacy Healer save format
		if (source.contains("amountToHeal"))
		{
			consumable.effect = ConsumableEffect::HEAL;
			consumable.amount = source["amountToHeal"].get<int>();
		}
		else if (source.contains("amount"))
		{
			consumable.amount = source["amount"].get<int>();
		}

		if (source.contains("duration"))
		{
			consumable.duration = source["duration"].get<int>();
		}
		if (source.contains("buffType"))
		{
			consumable.buffType = static_cast<BuffType>(source["buffType"].get<int>());
		}
		if (source.contains("isSetEffect"))
		{
			consumable.isSetEffect = source["isSetEffect"].get<bool>();
		}

		return consumable;
	}

	case PickableType::WEAPON:
	{
		Weapon weapon;
		if (source.contains("ranged"))
		{
			weapon.ranged = source["ranged"].get<bool>();
		}
		if (source.contains("handRequirement"))
		{
			weapon.handRequirement = static_cast<HandRequirement>(source["handRequirement"].get<int>());
		}
		if (source.contains("weaponSize"))
		{
			weapon.weaponSize = static_cast<WeaponSize>(source["weaponSize"].get<int>());
		}
		return weapon;
	}

	case PickableType::SHIELD:
	{
		return Shield{};
	}

	case PickableType::TARGETED_SCROLL:
	{
		TargetedScroll targetedScroll;
		if (source.contains("targetMode"))
		{
			targetedScroll.targetMode = static_cast<TargetMode>(source["targetMode"].get<int>());
		}
		if (source.contains("animation"))
		{
			targetedScroll.scrollAnimation = static_cast<ScrollAnimation>(source["animation"].get<int>());
		}
		if (source.contains("range"))
		{
			targetedScroll.range = source["range"].get<int>();
		}
		if (source.contains("damage"))
		{
			targetedScroll.damage = source["damage"].get<int>();
		}
		if (source.contains("confuseTurns"))
		{
			targetedScroll.confuseTurns = source["confuseTurns"].get<int>();
		}
		if (source.contains("buffType"))
		{
			targetedScroll.buffType = static_cast<BuffType>(source["buffType"].get<int>());
		}
		if (source.contains("buffDuration"))
		{
			targetedScroll.buffDuration = source["buffDuration"].get<int>();
		}
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
		if (source.contains("amount"))
		{
			gold.amount = source["amount"].get<int>();
		}
		return gold;
	}

	case PickableType::FOOD:
	{
		Food food;
		if (source.contains("nutritionValue"))
		{
			food.nutritionValue = source["nutritionValue"].get<int>();
		}
		return food;
	}

	case PickableType::CORPSE_FOOD:
	{
		CorpseFood corpseFood;
		if (source.contains("nutritionValue"))
		{
			corpseFood.nutritionValue = source["nutritionValue"].get<int>();
		}
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
		if (source.contains("effect"))
		{
			magicalHelm.effect = static_cast<MagicalEffect>(source["effect"].get<int>());
		}
		if (source.contains("bonus"))
		{
			magicalHelm.bonus = source["bonus"].get<int>();
		}
		return magicalHelm;
	}

	case PickableType::MAGICAL_RING:
	{
		MagicalRing magicalRing;
		if (source.contains("effect"))
		{
			magicalRing.effect = static_cast<MagicalEffect>(source["effect"].get<int>());
		}
		if (source.contains("bonus"))
		{
			magicalRing.bonus = source["bonus"].get<int>();
		}
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
		throw std::runtime_error(std::format("Unknown PickableType: {}", static_cast<int>(type)));
	}
	}
}
