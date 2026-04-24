// file: Pickable.cpp
#include <algorithm>
#include <format>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <variant>

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
#include "../Systems/RenderingManager.h"
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

Vector2D find_valid_teleport_location(GameContext& ctx)
{
	auto is_position_free = [&](int x, int y) -> bool
	{
		for (const auto& creature : *ctx.creatures)
		{
			if (creature && creature->position.x == x && creature->position.y == y)
				return false;
		}
		if (ctx.player && ctx.player->position.x == x && ctx.player->position.y == y)
			return false;
		return true;
	};

	for (int attempts = 0; attempts < 50; ++attempts)
	{
		const int x = ctx.dice->roll(2, ctx.map->get_width() - 2);
		const int y = ctx.dice->roll(2, ctx.map->get_height() - 2);

		if (ctx.map->get_tile_type(Vector2D{ x, y }) == TileType::FLOOR && is_position_free(x, y))
			return Vector2D{ x, y };
	}

	return Vector2D(-1, -1);
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
	const auto it = map.find(key);
	if (it != map.end())
		return it->second;
	return default_value;
}

// Shared save for stat-boost equipment (Gauntlets, Girdle, JewelryAmulet)
template <typename T>
void save_stat_boost(const T& sb, PickableType type, json& j)
{
	j["type"] = static_cast<int>(type);
	j["strBonus"] = sb.strBonus;
	j["dexBonus"] = sb.dexBonus;
	j["conBonus"] = sb.conBonus;
	j["intBonus"] = sb.intBonus;
	j["wisBonus"] = sb.wisBonus;
	j["chaBonus"] = sb.chaBonus;
	j["isSetMode"] = sb.isSetMode;
	j["originalStats"] = {
		{ "str", sb.originalStats.str },
		{ "dex", sb.originalStats.dex },
		{ "con", sb.originalStats.con },
		{ "intel", sb.originalStats.intel },
		{ "wis", sb.originalStats.wis },
		{ "cha", sb.originalStats.cha },
	};
}

template <typename T>
void load_stat_boost(T& sb, const json& j)
{
	sb.strBonus = j.contains("strBonus") ? j.at("strBonus").get<int>() : 0;
	sb.dexBonus = j.contains("dexBonus") ? j.at("dexBonus").get<int>() : 0;
	sb.conBonus = j.contains("conBonus") ? j.at("conBonus").get<int>() : 0;
	sb.intBonus = j.contains("intBonus") ? j.at("intBonus").get<int>() : 0;
	sb.wisBonus = j.contains("wisBonus") ? j.at("wisBonus").get<int>() : 0;
	sb.chaBonus = j.contains("chaBonus") ? j.at("chaBonus").get<int>() : 0;
	sb.isSetMode = j.contains("isSetMode") ? j.at("isSetMode").get<bool>() : false;

	if (j.contains("originalStats"))
	{
		const auto& orig = j.at("originalStats");
		sb.originalStats.str = orig.contains("str") ? orig.at("str").get<int>() : 0;
		sb.originalStats.dex = orig.contains("dex") ? orig.at("dex").get<int>() : 0;
		sb.originalStats.con = orig.contains("con") ? orig.at("con").get<int>() : 0;
		sb.originalStats.intel = orig.contains("intel") ? orig.at("intel").get<int>() : 0;
		sb.originalStats.wis = orig.contains("wis") ? orig.at("wis").get<int>() : 0;
		sb.originalStats.cha = orig.contains("cha") ? orig.at("cha").get<int>() : 0;
	}
}

// Shared use() for stat-boost equipment (Gauntlets, Girdle, JewelryAmulet)
template <typename T>
bool use_stat_boost(T& sb, EquipmentSlot slot, Item& item, Creature& wearer, GameContext& ctx)
{
	const bool wasEquipped = wearer.is_item_equipped(item.uniqueId);
	const bool success = wearer.toggle_equipment(item.uniqueId, slot, ctx);

	if (success)
	{
		if (wasEquipped)
		{
			if (sb.isSetMode)
			{
				if (sb.strBonus != 0)
				{
					wearer.set_strength(sb.originalStats.str);
				}
				if (sb.dexBonus != 0)
				{
					wearer.set_dexterity(sb.originalStats.dex);
				}
				if (sb.conBonus != 0)
				{
					wearer.set_constitution(sb.originalStats.con);
				}
				if (sb.intBonus != 0)
				{
					wearer.set_intelligence(sb.originalStats.intel);
				}
				if (sb.wisBonus != 0)
				{
					wearer.set_wisdom(sb.originalStats.wis);
				}
				if (sb.chaBonus != 0)
				{
					wearer.set_charisma(sb.originalStats.cha);
				}
			}
			else
			{
				wearer.set_strength(wearer.get_strength() - sb.strBonus);
				wearer.set_dexterity(wearer.get_dexterity() - sb.dexBonus);
				wearer.set_constitution(wearer.get_constitution() - sb.conBonus);
				wearer.set_intelligence(wearer.get_intelligence() - sb.intBonus);
				wearer.set_wisdom(wearer.get_wisdom() - sb.wisBonus);
				wearer.set_charisma(wearer.get_charisma() - sb.chaBonus);
			}

			wearer.destructible->update_armor_class(wearer, ctx);
			ctx.messageSystem->message(WHITE_BLACK_PAIR, "You remove the " + item.actorData.name + ".", true);
		}
		else
		{
			if (sb.isSetMode)
			{
				if (sb.strBonus != 0)
				{
					sb.originalStats.str = wearer.get_strength();
					wearer.set_strength(sb.strBonus);
				}
				if (sb.dexBonus != 0)
				{
					sb.originalStats.dex = wearer.get_dexterity();
					wearer.set_dexterity(sb.dexBonus);
				}
				if (sb.conBonus != 0)
				{
					sb.originalStats.con = wearer.get_constitution();
					wearer.set_constitution(sb.conBonus);
				}
				if (sb.intBonus != 0)
				{
					sb.originalStats.intel = wearer.get_intelligence();
					wearer.set_intelligence(sb.intBonus);
				}
				if (sb.wisBonus != 0)
				{
					sb.originalStats.wis = wearer.get_wisdom();
					wearer.set_wisdom(sb.wisBonus);
				}
				if (sb.chaBonus != 0)
				{
					sb.originalStats.cha = wearer.get_charisma();
					wearer.set_charisma(sb.chaBonus);
				}
			}
			else
			{
				wearer.set_strength(wearer.get_strength() + sb.strBonus);
				wearer.set_dexterity(wearer.get_dexterity() + sb.dexBonus);
				wearer.set_constitution(wearer.get_constitution() + sb.conBonus);
				wearer.set_intelligence(wearer.get_intelligence() + sb.intBonus);
				wearer.set_wisdom(wearer.get_wisdom() + sb.wisBonus);
				wearer.set_charisma(wearer.get_charisma() + sb.chaBonus);
			}

			wearer.destructible->update_armor_class(wearer, ctx);
			ctx.messageSystem->message(WHITE_BLACK_PAIR, "You put on the " + item.actorData.name + ".", true);
		}
		return true;
	}

	// NPC fallback: toggle equipped state + apply bonuses directly
	if (item.has_state(ActorState::IS_EQUIPPED))
	{
		item.remove_state(ActorState::IS_EQUIPPED);
		if (sb.isSetMode)
		{
			if (sb.strBonus != 0)
				wearer.set_strength(sb.originalStats.str);
			if (sb.dexBonus != 0)
				wearer.set_dexterity(sb.originalStats.dex);
			if (sb.conBonus != 0)
				wearer.set_constitution(sb.originalStats.con);
			if (sb.intBonus != 0)
				wearer.set_intelligence(sb.originalStats.intel);
			if (sb.wisBonus != 0)
				wearer.set_wisdom(sb.originalStats.wis);
			if (sb.chaBonus != 0)
				wearer.set_charisma(sb.originalStats.cha);
		}
		else
		{
			wearer.set_strength(wearer.get_strength() - sb.strBonus);
			wearer.set_dexterity(wearer.get_dexterity() - sb.dexBonus);
			wearer.set_constitution(wearer.get_constitution() - sb.conBonus);
			wearer.set_intelligence(wearer.get_intelligence() - sb.intBonus);
			wearer.set_wisdom(wearer.get_wisdom() - sb.wisBonus);
			wearer.set_charisma(wearer.get_charisma() - sb.chaBonus);
		}
		wearer.destructible->update_armor_class(wearer, ctx);
	}
	else
	{
		item.add_state(ActorState::IS_EQUIPPED);
		if (sb.isSetMode)
		{
			if (sb.strBonus != 0)
			{
				sb.originalStats.str = wearer.get_strength();
				wearer.set_strength(sb.strBonus);
			}
			if (sb.dexBonus != 0)
			{
				sb.originalStats.dex = wearer.get_dexterity();
				wearer.set_dexterity(sb.dexBonus);
			}
			if (sb.conBonus != 0)
			{
				sb.originalStats.con = wearer.get_constitution();
				wearer.set_constitution(sb.conBonus);
			}
			if (sb.intBonus != 0)
			{
				sb.originalStats.intel = wearer.get_intelligence();
				wearer.set_intelligence(sb.intBonus);
			}
			if (sb.wisBonus != 0)
			{
				sb.originalStats.wis = wearer.get_wisdom();
				wearer.set_wisdom(sb.wisBonus);
			}
			if (sb.chaBonus != 0)
			{
				sb.originalStats.cha = wearer.get_charisma();
				wearer.set_charisma(sb.chaBonus);
			}
		}
		else
		{
			wearer.set_strength(wearer.get_strength() + sb.strBonus);
			wearer.set_dexterity(wearer.get_dexterity() + sb.dexBonus);
			wearer.set_constitution(wearer.get_constitution() + sb.conBonus);
			wearer.set_intelligence(wearer.get_intelligence() + sb.intBonus);
			wearer.set_wisdom(wearer.get_wisdom() + sb.wisBonus);
			wearer.set_charisma(wearer.get_charisma() + sb.chaBonus);
		}
		wearer.destructible->update_armor_class(wearer, ctx);
	}

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
			targetSlot = EquipmentSlot::LEFT_RING;
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
				ctx.messageSystem->message(CYAN_BLACK_PAIR, "The ring pulses with arcane power. Press Ctrl+C to cast.", true);
			ctx.messageSystem->message(WHITE_BLACK_PAIR, "You put on the " + item.actorData.name + ".", true);
		}

		if (MagicalEffectUtils::is_protection_effect(effect) || effect == MagicalEffect::BRILLIANCE)
			wearer.destructible->update_armor_class(wearer, ctx);

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

bool Weapon::validate_dual_wield(const Item* main_hand, const Item* off_hand) const
{
	if (!main_hand || !off_hand)
	{
		return false;
	}
	return main_hand->is_weapon() && off_hand->is_weapon();
}

EquipmentSlot Weapon::get_preferred_slot(const Creature* creature) const
{
	if (ranged)
	{
		return EquipmentSlot::MISSILE_WEAPON;
	}

	if (!can_be_off_hand())
	{
		return EquipmentSlot::RIGHT_HAND;
	}

	Item* main_hand = creature->get_equipped_item(EquipmentSlot::RIGHT_HAND);
	Item* off_hand = creature->get_equipped_item(EquipmentSlot::LEFT_HAND);

	if (!main_hand || off_hand)
	{
		return EquipmentSlot::RIGHT_HAND;
	}

	if (main_hand->is_weapon())
	{
		if (const Weapon* main_weapon = std::get_if<Weapon>(&*main_hand->behavior))
		{
			if (main_weapon->get_weapon_size() > weaponSize)
			{
				return EquipmentSlot::LEFT_HAND;
			}
		}
	}

	return EquipmentSlot::RIGHT_HAND;
}

// ========== use() implementations ==========

bool use(Consumable& c, Item& owner, Creature& wearer, GameContext& ctx)
{
	switch (c.effect)
	{

	case ConsumableEffect::HEAL:
	{
		if (!wearer.destructible)
		{
			return false;
		}

		if (wearer.destructible->get_hp() >= wearer.destructible->get_max_hp())
		{
			ctx.messageSystem->message(WHITE_BLACK_PAIR, "You are already at full health.", true);
			return false;
		}
		const int healed = wearer.destructible->heal(c.amount);
		ctx.messageSystem->message(GREEN_BLACK_PAIR, std::format("You feel better! (+{} HP)", healed), true);
		break;
	}

	case ConsumableEffect::ADD_BUFF:
	{
		ctx.buffSystem->add_buff(wearer, c.buffType, c.amount, c.duration, c.isSetEffect);
		ctx.messageSystem->message(
			CYAN_BLACK_PAIR,
			std::format("You feel the effect of the {} for {} turns.", owner.get_name(), c.duration),
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

bool use(Weapon& w, Item& owner, Creature& wearer, GameContext& ctx)
{
	const EquipmentSlot preferred = w.get_preferred_slot(&wearer);
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
			if (!creature || !creature->destructible || creature->destructible->is_dead())
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
		TargetResult result = ctx.targeting->acquire_targets(
			ctx,
			TargetMode::AUTO_NEAREST,
			wearer.position,
			targetScroll.range,
			0);
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
		target->destructible->take_damage(*target, targetScroll.damage, ctx);
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

			if (innerCtx.player->get_tile_distance(targetPos) <= aoeRadius)
			{
				SpellAnimations::animate_creature_hit(innerCtx.player->position, innerCtx);
				innerCtx.player->destructible->take_damage(*innerCtx.player, scrollDamage, innerCtx);
			}

			for (const auto& creature : *innerCtx.creatures)
			{
				if (!creature || creature->destructible->is_dead())
					continue;
				if (creature->get_tile_distance(targetPos) > aoeRadius)
					continue;
				SpellAnimations::animate_creature_hit(creature->position, innerCtx);
				innerCtx.messageSystem->append_message_part(
					WHITE_BLACK_PAIR,
					std::format("The {} gets engulfed in flames! ({} damage)", creature->actorData.name, scrollDamage));
				innerCtx.messageSystem->finalize_message();
				creature->destructible->take_damage(*creature, scrollDamage, innerCtx);
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

bool use(IdentifyScroll& idScroll, Item& owner, Creature& wearer, GameContext& ctx)
{
	(void)idScroll; // unused parameter

	auto* player = dynamic_cast<Player*>(&wearer);
	if (!player)
	{
		return false;
	}

	int identifiedCount = 0;
	for (auto& item : player->inventoryData.items)
	{
		if (!item->is_fully_identified())
		{
			item->identify_all();
			identifiedCount++;
			if (ctx.floatingText)
			{
				std::string identified_msg = std::string(item->get_name()) + " identified!";
				ctx.floatingText->spawn_text(
					player->position.x,
					player->position.y,
					identified_msg,
					0,
					255,
					255,
					2.0f);
			}
		}
	}

	if (identifiedCount > 0)
	{
		ctx.messageSystem->message(
			CYAN_BLACK_PAIR,
			std::format("You use the {}. {} items identified!", owner.get_name(), identifiedCount),
			true);
	}
	else
	{
		ctx.messageSystem->message(
			CYAN_BLACK_PAIR,
			std::format("You use the {}. All items were already identified.", owner.get_name()),
			true);
	}

	return consume_item(owner, wearer);
}

bool use(Gold& g, Item& owner, Creature& wearer, GameContext& ctx)
{
	wearer.adjust_gold(g.amount);
	ctx.messageSystem->append_message_part(YELLOW_BLACK_PAIR, "You gained ");
	ctx.messageSystem->append_message_part(YELLOW_BLACK_PAIR, std::to_string(g.amount));
	ctx.messageSystem->append_message_part(YELLOW_BLACK_PAIR, " gold.");
	ctx.messageSystem->finalize_message();
	return consume_item(owner, wearer);
}

bool use(Food& f, Item& owner, Creature& wearer, GameContext& ctx)
{
	ctx.hungerSystem->decrease_hunger(ctx, f.nutritionValue);
	ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, "You eat the ");
	ctx.messageSystem->append_message_part(YELLOW_BLACK_PAIR, owner.actorData.name);
	ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, ".");
	ctx.messageSystem->finalize_message();
	return consume_item(owner, wearer);
}

bool use(CorpseFood& cf, Item& owner, Creature& wearer, GameContext& ctx)
{
	if (cf.nutritionValue <= 0)
	{
		cf.nutritionValue = get_or_default(corpseNutritionValues, owner.actorData.name, 50);
	}

	int actual = cf.nutritionValue + ctx.dice->roll(-10, 10);
	actual = std::max(10, actual);

	ctx.hungerSystem->decrease_hunger(ctx, actual);

	const std::string& flavor = get_or_default(corpseFlavorText, owner.actorData.name, std::string{ "It tastes... questionable." });

	ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, "You eat the ");
	ctx.messageSystem->append_message_part(RED_BLACK_PAIR, owner.actorData.name);
	ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, ". " + flavor);
	ctx.messageSystem->finalize_message();
	return consume_item(owner, wearer);
}

bool use([[maybe_unused]] Armor& a, Item& item, Creature& wearer, GameContext& ctx)
{
	if (wearer.is_player())
	{
		const bool was_equipped = wearer.is_item_equipped(item.uniqueId);

		// static_cast safe: is_player() guarantees type
		auto* player = static_cast<Player*>(&wearer);
		const bool success = player->toggle_armor(item.uniqueId, ctx);

		if (success)
		{
			if (was_equipped)
			{
				ctx.messageSystem->message(WHITE_BLACK_PAIR, "You remove the " + item.actorData.name + ".", true);
			}
			else
			{
				ctx.messageSystem->message(WHITE_BLACK_PAIR, "You put on the " + item.actorData.name + ".", true);
			}
		}
		return success;
	}

	// NPC fallback: toggle equipped state
	if (item.has_state(ActorState::IS_EQUIPPED))
	{
		item.remove_state(ActorState::IS_EQUIPPED);
		wearer.destructible->update_armor_class(wearer, ctx);
	}
	else
	{
		item.add_state(ActorState::IS_EQUIPPED);
		wearer.destructible->update_armor_class(wearer, ctx);
	}
	return true;
}

bool use(MagicalHelm& mh, Item& owner, Creature& wearer, GameContext& ctx)
{
	return use_magical_equip(mh.effect, EquipmentSlot::HEAD, owner, wearer, ctx);
}

bool use(MagicalRing& mr, Item& owner, Creature& wearer, GameContext& ctx)
{
	return use_magical_equip(mr.effect, EquipmentSlot::RIGHT_RING, owner, wearer, ctx);
}

bool use(JewelryAmulet& ja, Item& owner, Creature& wearer, GameContext& ctx)
{
	return use_stat_boost(ja, EquipmentSlot::NECK, owner, wearer, ctx);
}

bool use(Gauntlets& g, Item& owner, Creature& wearer, GameContext& ctx)
{
	return use_stat_boost(g, EquipmentSlot::GAUNTLETS, owner, wearer, ctx);
}

bool use(Girdle& g, Item& owner, Creature& wearer, GameContext& ctx)
{
	return use_stat_boost(g, EquipmentSlot::GIRDLE, owner, wearer, ctx);
}

bool use(DungeonKey& /*dk*/, Item& /*owner*/, Creature& /*wearer*/, GameContext& ctx)
{
	ctx.messageSystem->message(WHITE_BLACK_PAIR, "Bump into a locked door to use this key.", true);
	return false;
}

// ========== Variant-level dispatchers ==========

bool use_item(ItemBehavior& behavior, Item& owner, Creature& wearer, GameContext& ctx)
{
	return std::visit(
		[&owner, &wearer, &ctx](auto& b) -> bool
		{
			using T = std::decay_t<decltype(b)>;

			// Empty-struct types carry no use-site data -- behavior lives here.
			if constexpr (std::is_same_v<T, Shield>)
			{
				const bool success = wearer.toggle_shield(owner.uniqueId, ctx);
				if (success)
				{
					Item* equipped = wearer.get_equipped_item(EquipmentSlot::LEFT_HAND);
					if (equipped && equipped->uniqueId == owner.uniqueId)
						ctx.messageSystem->message(WHITE_BLACK_PAIR, std::format("You raise the {}.", owner.get_name()), true);
					else
						ctx.messageSystem->message(WHITE_BLACK_PAIR, std::format("You lower the {}.", owner.get_name()), true);
					return true;
				}
				wearer.equip(owner, ctx);
				return true;
			}
			else if constexpr (std::is_same_v<T, Teleporter>)
			{
				const Vector2D validLocation = find_valid_teleport_location(ctx);
				if (validLocation.x != -1 && validLocation.y != -1)
				{
					wearer.position = validLocation;
					ctx.map->compute_fov(ctx);
					ctx.messageSystem->message(BLUE_BLACK_PAIR, "You feel disoriented as the world shifts around you!", true);
					ctx.messageSystem->message(WHITE_BLACK_PAIR, "You have been teleported to a new location.", true);
					return consume_item(owner, wearer);
				}
				ctx.messageSystem->message(RED_BLACK_PAIR, "The teleportation magic fizzles out - no safe location found!", true);
				return false;
			}
			else if constexpr (std::is_same_v<T, Amulet>)
			{
				ctx.messageSystem->message(WHITE_BLACK_PAIR, "The Amulet of Yendor glows brightly in your hands!", true);
				ctx.messageSystem->message(WHITE_BLACK_PAIR, "You feel a powerful magic enveloping you...", true);
				ctx.gameState->set_game_status(GameStatus::VICTORY);
				return false;
			}
			else
			{
				return use(b, owner, wearer, ctx);
			}
		},
		behavior);
}

int get_item_ac_bonus(const ItemBehavior& behavior) noexcept
{
	return std::visit(
		[](const auto& b) -> int
		{
			using T = std::decay_t<decltype(b)>;
			if constexpr (std::is_same_v<T, Armor>)
				return b.armorClass;
			else if constexpr (std::is_same_v<T, Shield>)
				return -1; // +1 AC in AD&D terms
			else if constexpr (std::is_same_v<T, MagicalHelm>)
				return MagicalEffectUtils::get_ac_bonus(b.effect, b.bonus);
			else if constexpr (std::is_same_v<T, MagicalRing>)
				return MagicalEffectUtils::get_protection_bonus(b.effect);
			else
				return 0;
		},
		behavior);
}

// ========== Serialization ==========

void save_behavior(const ItemBehavior& behavior, json& j)
{
	std::visit(
		[&j](const auto& b)
		{
			using T = std::decay_t<decltype(b)>;

			if constexpr (std::is_same_v<T, Consumable>)
			{
				j["type"] = static_cast<int>(PickableType::CONSUMABLE);
				j["effect"] = static_cast<int>(b.effect);
				j["amount"] = b.amount;
				j["duration"] = b.duration;
				j["buffType"] = static_cast<int>(b.buffType);
				j["isSetEffect"] = b.isSetEffect;
			}
			else if constexpr (std::is_same_v<T, Weapon>)
			{
				j["type"] = static_cast<int>(PickableType::WEAPON);
				j["ranged"] = b.ranged;
				j["handReq"] = static_cast<int>(b.handRequirement);
				j["weaponSize"] = static_cast<int>(b.weaponSize);
			}
			else if constexpr (std::is_same_v<T, Shield>)
			{
				j["type"] = static_cast<int>(PickableType::SHIELD);
			}
			else if constexpr (std::is_same_v<T, TargetedScroll>)
			{
				j["type"] = static_cast<int>(PickableType::TARGETED_SCROLL);
				j["targetMode"] = static_cast<int>(b.targetMode);
				j["scrollAnimation"] = static_cast<int>(b.scrollAnimation);
				j["range"] = b.range;
				j["damage"] = b.damage;
				j["confuseTurns"] = b.confuseTurns;
				j["buffType"] = static_cast<int>(b.buffType);
				j["buffDuration"] = b.buffDuration;
			}
			else if constexpr (std::is_same_v<T, Teleporter>)
			{
				j["type"] = static_cast<int>(PickableType::TELEPORTER);
			}
			else if constexpr (std::is_same_v<T, IdentifyScroll>)
			{
				j["type"] = static_cast<int>(PickableType::IDENTIFY_SCROLL);
			}
			else if constexpr (std::is_same_v<T, Gold>)
			{
				j["type"] = static_cast<int>(PickableType::GOLD_COIN);
				j["amount"] = b.amount;
			}
			else if constexpr (std::is_same_v<T, Food>)
			{
				j["type"] = static_cast<int>(PickableType::FOOD);
				j["nutritionValue"] = b.nutritionValue;
			}
			else if constexpr (std::is_same_v<T, CorpseFood>)
			{
				j["type"] = static_cast<int>(PickableType::CORPSE_FOOD);
				j["nutritionValue"] = b.nutritionValue;
			}
			else if constexpr (std::is_same_v<T, Armor>)
			{
				j["type"] = static_cast<int>(PickableType::ARMOR);
				j["armorClass"] = b.armorClass;
			}
			else if constexpr (std::is_same_v<T, MagicalHelm>)
			{
				j["type"] = static_cast<int>(PickableType::MAGICAL_HELM);
				j["effect"] = static_cast<int>(b.effect);
				j["bonus"] = b.bonus;
			}
			else if constexpr (std::is_same_v<T, MagicalRing>)
			{
				j["type"] = static_cast<int>(PickableType::MAGICAL_RING);
				j["effect"] = static_cast<int>(b.effect);
				j["bonus"] = b.bonus;
			}
			else if constexpr (std::is_same_v<T, JewelryAmulet>)
			{
				save_stat_boost(b, PickableType::JEWELRY_AMULET, j);
			}
			else if constexpr (std::is_same_v<T, Gauntlets>)
			{
				save_stat_boost(b, PickableType::GAUNTLETS, j);
			}
			else if constexpr (std::is_same_v<T, Girdle>)
			{
				save_stat_boost(b, PickableType::GIRDLE, j);
			}
			else if constexpr (std::is_same_v<T, Amulet>)
			{
				j["type"] = static_cast<int>(PickableType::QUEST_ITEM);
			}
			else if constexpr (std::is_same_v<T, DungeonKey>)
			{
				j["type"] = static_cast<int>(PickableType::DUNGEON_KEY);
			}
		},
		behavior);
}

ItemBehavior load_behavior(const json& j)
{
	if (!j.contains("type") || !j["type"].is_number())
	{
		throw std::runtime_error("Invalid JSON format: Missing or invalid 'type'");
	}

	const auto type = static_cast<PickableType>(j["type"].get<int>());

	switch (type)
	{

	case PickableType::CONSUMABLE:
	{
		Consumable c;
		if (j.contains("effect"))
		{
			c.effect = static_cast<ConsumableEffect>(j["effect"].get<int>());
		}

		// Legacy Healer save format
		if (j.contains("amountToHeal"))
		{
			c.effect = ConsumableEffect::HEAL;
			c.amount = j["amountToHeal"].get<int>();
		}
		else if (j.contains("amount"))
		{
			c.amount = j["amount"].get<int>();
		}

		if (j.contains("duration"))
		{
			c.duration = j["duration"].get<int>();
		}
		if (j.contains("buffType"))
		{
			c.buffType = static_cast<BuffType>(j["buffType"].get<int>());
		}
		if (j.contains("isSetEffect"))
		{
			c.isSetEffect = j["isSetEffect"].get<bool>();
		}

		return c;
	}

	case PickableType::WEAPON:
	{
		Weapon w;
		if (j.contains("ranged"))
		{
			w.ranged = j["ranged"].get<bool>();
		}
		if (j.contains("handRequirement"))
		{
			w.handRequirement = static_cast<HandRequirement>(j["handRequirement"].get<int>());
		}
		if (j.contains("weaponSize"))
		{
			w.weaponSize = static_cast<WeaponSize>(j["weaponSize"].get<int>());
		}
		return w;
	}

	case PickableType::SHIELD:
	{
		return Shield{};
	}

	case PickableType::TARGETED_SCROLL:
	{
		TargetedScroll targetedScroll;
		if (j.contains("targetMode"))
		{
			targetedScroll.targetMode = static_cast<TargetMode>(j["targetMode"].get<int>());
		}
		if (j.contains("animation"))
		{
			targetedScroll.scrollAnimation = static_cast<ScrollAnimation>(j["animation"].get<int>());
		}
		if (j.contains("range"))
		{
			targetedScroll.range = j["range"].get<int>();
		}
		if (j.contains("damage"))
		{
			targetedScroll.damage = j["damage"].get<int>();
		}
		if (j.contains("confuseTurns"))
		{
			targetedScroll.confuseTurns = j["confuseTurns"].get<int>();
		}
		if (j.contains("buffType"))
		{
			targetedScroll.buffType = static_cast<BuffType>(j["buffType"].get<int>());
		}
		if (j.contains("buffDuration"))
		{
			targetedScroll.buffDuration = j["buffDuration"].get<int>();
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
		Gold g;
		if (j.contains("amount"))
			g.amount = j["amount"].get<int>();
		return g;
	}

	case PickableType::FOOD:
	{
		Food f;
		if (j.contains("nutritionValue"))
			f.nutritionValue = j["nutritionValue"].get<int>();
		return f;
	}

	case PickableType::CORPSE_FOOD:
	{
		CorpseFood cf;
		if (j.contains("nutritionValue"))
			cf.nutritionValue = j["nutritionValue"].get<int>();
		return cf;
	}

	case PickableType::ARMOR:
	{
		Armor a;
		a.armorClass = j.at("armorClass").get<int>();
		return a;
	}

	case PickableType::MAGICAL_HELM:
	{
		MagicalHelm mh;
		if (j.contains("effect"))
			mh.effect = static_cast<MagicalEffect>(j["effect"].get<int>());
		if (j.contains("bonus"))
			mh.bonus = j["bonus"].get<int>();
		return mh;
	}

	case PickableType::MAGICAL_RING:
	{
		MagicalRing mr;
		if (j.contains("effect"))
		{
			mr.effect = static_cast<MagicalEffect>(j["effect"].get<int>());
		}
		if (j.contains("bonus"))
		{
			mr.bonus = j["bonus"].get<int>();
		}
		return mr;
	}

	case PickableType::JEWELRY_AMULET:
	{
		JewelryAmulet ja;
		load_stat_boost(ja, j);
		return ja;
	}

	case PickableType::GAUNTLETS:
	{
		Gauntlets g;
		load_stat_boost(g, j);
		return g;
	}

	case PickableType::GIRDLE:
	{
		Girdle g;
		load_stat_boost(g, j);
		return g;
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
