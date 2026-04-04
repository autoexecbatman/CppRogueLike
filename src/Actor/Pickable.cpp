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
#include "../Systems/HungerSystem.h"
#include "../Systems/MessageSystem.h"
#include "../Systems/RenderingManager.h"
#include "../Systems/SpellAnimations.h"
#include "../Systems/TargetingSystem.h"
#include "../Systems/TargetMode.h"
#include "../Utils/Vector2D.h"
#include "Actor.h"
#include "EquipmentSlot.h"
#include "InventoryData.h"
#include "InventoryOperations.h"
#include "Pickable.h"

using namespace InventoryOperations;

// ========== Internal helpers ==========

namespace
{

bool consume_item(Item& owner, Creature& wearer)
{
	auto result = remove_item(wearer.inventory_data, owner);
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

const std::unordered_map<std::string, int> CORPSE_NUTRITION_VALUES = {
	{ "dead goblin", 40 },
	{ "dead orc", 80 },
	{ "dead troll", 120 },
	{ "dead dragon", 200 },
	{ "dead archer", 70 },
	{ "dead mage", 60 },
	{ "dead shopkeeper", 100 },
};

const std::unordered_map<std::string, std::string> CORPSE_FLAVOR_TEXT = {
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
	j["str_bonus"] = sb.str_bonus;
	j["dex_bonus"] = sb.dex_bonus;
	j["con_bonus"] = sb.con_bonus;
	j["int_bonus"] = sb.int_bonus;
	j["wis_bonus"] = sb.wis_bonus;
	j["cha_bonus"] = sb.cha_bonus;
	j["is_set_mode"] = sb.is_set_mode;
	j["original_stats"] = {
		{ "str", sb.original_stats.str },
		{ "dex", sb.original_stats.dex },
		{ "con", sb.original_stats.con },
		{ "intel", sb.original_stats.intel },
		{ "wis", sb.original_stats.wis },
		{ "cha", sb.original_stats.cha },
	};
}

template <typename T>
void load_stat_boost(T& sb, const json& j)
{
	sb.str_bonus = j.contains("str_bonus") ? j.at("str_bonus").get<int>() : 0;
	sb.dex_bonus = j.contains("dex_bonus") ? j.at("dex_bonus").get<int>() : 0;
	sb.con_bonus = j.contains("con_bonus") ? j.at("con_bonus").get<int>() : 0;
	sb.int_bonus = j.contains("int_bonus") ? j.at("int_bonus").get<int>() : 0;
	sb.wis_bonus = j.contains("wis_bonus") ? j.at("wis_bonus").get<int>() : 0;
	sb.cha_bonus = j.contains("cha_bonus") ? j.at("cha_bonus").get<int>() : 0;
	sb.is_set_mode = j.contains("is_set_mode") ? j.at("is_set_mode").get<bool>() : false;

	if (j.contains("original_stats"))
	{
		const auto& orig = j.at("original_stats");
		sb.original_stats.str = orig.contains("str") ? orig.at("str").get<int>() : 0;
		sb.original_stats.dex = orig.contains("dex") ? orig.at("dex").get<int>() : 0;
		sb.original_stats.con = orig.contains("con") ? orig.at("con").get<int>() : 0;
		sb.original_stats.intel = orig.contains("intel") ? orig.at("intel").get<int>() : 0;
		sb.original_stats.wis = orig.contains("wis") ? orig.at("wis").get<int>() : 0;
		sb.original_stats.cha = orig.contains("cha") ? orig.at("cha").get<int>() : 0;
	}
}

// Shared use() for stat-boost equipment (Gauntlets, Girdle, JewelryAmulet)
template <typename T>
bool use_stat_boost(T& sb, EquipmentSlot slot, Item& item, Creature& wearer, GameContext& ctx)
{
	const bool was_equipped = wearer.is_item_equipped(item.uniqueId);
	const bool success = wearer.toggle_equipment(item.uniqueId, slot, ctx);

	if (success)
	{
		if (was_equipped)
		{
			if (sb.is_set_mode)
			{
				if (sb.str_bonus != 0)
					wearer.set_strength(sb.original_stats.str);
				if (sb.dex_bonus != 0)
					wearer.set_dexterity(sb.original_stats.dex);
				if (sb.con_bonus != 0)
					wearer.set_constitution(sb.original_stats.con);
				if (sb.int_bonus != 0)
					wearer.set_intelligence(sb.original_stats.intel);
				if (sb.wis_bonus != 0)
					wearer.set_wisdom(sb.original_stats.wis);
				if (sb.cha_bonus != 0)
					wearer.set_charisma(sb.original_stats.cha);
			}
			else
			{
				wearer.set_strength(wearer.get_strength() - sb.str_bonus);
				wearer.set_dexterity(wearer.get_dexterity() - sb.dex_bonus);
				wearer.set_constitution(wearer.get_constitution() - sb.con_bonus);
				wearer.set_intelligence(wearer.get_intelligence() - sb.int_bonus);
				wearer.set_wisdom(wearer.get_wisdom() - sb.wis_bonus);
				wearer.set_charisma(wearer.get_charisma() - sb.cha_bonus);
			}

			wearer.destructible->update_armor_class(wearer, ctx);
			ctx.messageSystem->message(WHITE_BLACK_PAIR, "You remove the " + item.actorData.name + ".", true);
		}
		else
		{
			if (sb.is_set_mode)
			{
				if (sb.str_bonus != 0)
				{
					sb.original_stats.str = wearer.get_strength();
					wearer.set_strength(sb.str_bonus);
				}
				if (sb.dex_bonus != 0)
				{
					sb.original_stats.dex = wearer.get_dexterity();
					wearer.set_dexterity(sb.dex_bonus);
				}
				if (sb.con_bonus != 0)
				{
					sb.original_stats.con = wearer.get_constitution();
					wearer.set_constitution(sb.con_bonus);
				}
				if (sb.int_bonus != 0)
				{
					sb.original_stats.intel = wearer.get_intelligence();
					wearer.set_intelligence(sb.int_bonus);
				}
				if (sb.wis_bonus != 0)
				{
					sb.original_stats.wis = wearer.get_wisdom();
					wearer.set_wisdom(sb.wis_bonus);
				}
				if (sb.cha_bonus != 0)
				{
					sb.original_stats.cha = wearer.get_charisma();
					wearer.set_charisma(sb.cha_bonus);
				}
			}
			else
			{
				wearer.set_strength(wearer.get_strength() + sb.str_bonus);
				wearer.set_dexterity(wearer.get_dexterity() + sb.dex_bonus);
				wearer.set_constitution(wearer.get_constitution() + sb.con_bonus);
				wearer.set_intelligence(wearer.get_intelligence() + sb.int_bonus);
				wearer.set_wisdom(wearer.get_wisdom() + sb.wis_bonus);
				wearer.set_charisma(wearer.get_charisma() + sb.cha_bonus);
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
		if (sb.is_set_mode)
		{
			if (sb.str_bonus != 0)
				wearer.set_strength(sb.original_stats.str);
			if (sb.dex_bonus != 0)
				wearer.set_dexterity(sb.original_stats.dex);
			if (sb.con_bonus != 0)
				wearer.set_constitution(sb.original_stats.con);
			if (sb.int_bonus != 0)
				wearer.set_intelligence(sb.original_stats.intel);
			if (sb.wis_bonus != 0)
				wearer.set_wisdom(sb.original_stats.wis);
			if (sb.cha_bonus != 0)
				wearer.set_charisma(sb.original_stats.cha);
		}
		else
		{
			wearer.set_strength(wearer.get_strength() - sb.str_bonus);
			wearer.set_dexterity(wearer.get_dexterity() - sb.dex_bonus);
			wearer.set_constitution(wearer.get_constitution() - sb.con_bonus);
			wearer.set_intelligence(wearer.get_intelligence() - sb.int_bonus);
			wearer.set_wisdom(wearer.get_wisdom() - sb.wis_bonus);
			wearer.set_charisma(wearer.get_charisma() - sb.cha_bonus);
		}
		wearer.destructible->update_armor_class(wearer, ctx);
	}
	else
	{
		item.add_state(ActorState::IS_EQUIPPED);
		if (sb.is_set_mode)
		{
			if (sb.str_bonus != 0)
			{
				sb.original_stats.str = wearer.get_strength();
				wearer.set_strength(sb.str_bonus);
			}
			if (sb.dex_bonus != 0)
			{
				sb.original_stats.dex = wearer.get_dexterity();
				wearer.set_dexterity(sb.dex_bonus);
			}
			if (sb.con_bonus != 0)
			{
				sb.original_stats.con = wearer.get_constitution();
				wearer.set_constitution(sb.con_bonus);
			}
			if (sb.int_bonus != 0)
			{
				sb.original_stats.intel = wearer.get_intelligence();
				wearer.set_intelligence(sb.int_bonus);
			}
			if (sb.wis_bonus != 0)
			{
				sb.original_stats.wis = wearer.get_wisdom();
				wearer.set_wisdom(sb.wis_bonus);
			}
			if (sb.cha_bonus != 0)
			{
				sb.original_stats.cha = wearer.get_charisma();
				wearer.set_charisma(sb.cha_bonus);
			}
		}
		else
		{
			wearer.set_strength(wearer.get_strength() + sb.str_bonus);
			wearer.set_dexterity(wearer.get_dexterity() + sb.dex_bonus);
			wearer.set_constitution(wearer.get_constitution() + sb.con_bonus);
			wearer.set_intelligence(wearer.get_intelligence() + sb.int_bonus);
			wearer.set_wisdom(wearer.get_wisdom() + sb.wis_bonus);
			wearer.set_charisma(wearer.get_charisma() + sb.cha_bonus);
		}
		wearer.destructible->update_armor_class(wearer, ctx);
	}

	return true;
}

// Shared use() for magical equipment (MagicalHelm, MagicalRing)
bool use_magical_equip(MagicalEffect effect, EquipmentSlot slot, Item& item, Creature& wearer, GameContext& ctx)
{
	const bool was_equipped = wearer.is_item_equipped(item.uniqueId);
	EquipmentSlot target_slot = slot;

	// Rings: auto-choose open slot
	if (slot == EquipmentSlot::RIGHT_RING && !was_equipped)
	{
		if (wearer.is_slot_occupied(EquipmentSlot::RIGHT_RING))
			target_slot = EquipmentSlot::LEFT_RING;
	}

	const bool success = wearer.toggle_equipment(item.uniqueId, target_slot, ctx);
	if (success)
	{
		if (was_equipped)
		{
			if (effect == MagicalEffect::INVISIBILITY && wearer.is_invisible())
			{
				ctx.buff_system->remove_buff(wearer, BuffType::INVISIBILITY);
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
		item.add_state(ActorState::IS_EQUIPPED);
	else
		item.remove_state(ActorState::IS_EQUIPPED);

	return true;
}

} // namespace

// ========== Weapon struct methods ==========

bool Weapon::validate_dual_wield(const Item* main_hand, const Item* off_hand) const
{
	if (!main_hand || !off_hand)
		return false;
	return main_hand->is_weapon() && off_hand->is_weapon();
}

EquipmentSlot Weapon::get_preferred_slot(const Creature* creature) const
{
	if (ranged)
		return EquipmentSlot::MISSILE_WEAPON;

	if (!can_be_off_hand())
		return EquipmentSlot::RIGHT_HAND;

	Item* main_hand = creature->get_equipped_item(EquipmentSlot::RIGHT_HAND);
	Item* off_hand = creature->get_equipped_item(EquipmentSlot::LEFT_HAND);

	if (!main_hand || off_hand)
		return EquipmentSlot::RIGHT_HAND;

	if (main_hand->is_weapon())
	{
		if (const Weapon* main_weapon = std::get_if<Weapon>(&*main_hand->behavior))
		{
			if (main_weapon->get_weapon_size() > weapon_size)
				return EquipmentSlot::LEFT_HAND;
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
			return false;
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
		ctx.buff_system->add_buff(wearer, c.buff_type, c.amount, c.duration, c.is_set_effect);
		ctx.messageSystem->message(
			CYAN_BLACK_PAIR,
			std::format("You feel the effect of the {} for {} turns.", owner.get_name(), c.duration),
			true);
		break;
	case ConsumableEffect::NONE:
		ctx.messageSystem->message(WHITE_BLACK_PAIR, std::format("You use the {}.", owner.get_name()), true);
		break;
	case ConsumableEffect::FAIL:
		ctx.messageSystem->message(RED_BLACK_PAIR, std::format("Nothing happens with the {}.", owner.get_name()), true);
		return false;
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
			const std::string slot_name = (preferred == EquipmentSlot::LEFT_HAND) ? "off-hand" : "main hand";
			ctx.messageSystem->message(
				WHITE_BLACK_PAIR,
				std::format("You equip the {} in your {}.", owner.get_name(), slot_name),
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

bool use(Shield& s, Item& owner, Creature& wearer, GameContext& ctx)
{
	(void)s;
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

bool use(TargetedScroll& targetScroll, Item& owner, Creature& wearer, GameContext& ctx)
{
	if (targetScroll.target_mode == TargetMode::FOV_BUFF)
	{
		int affected = 0;
		for (const auto& creature : *ctx.creatures)
		{
			if (!creature || !creature->destructible || creature->destructible->is_dead())
				continue;
			if (!ctx.map->is_in_fov(creature->position))
				continue;
			const int save = ctx.dice->roll(1, 20);
			if (save < 15)
			{
				ctx.buff_system->add_buff(*creature, targetScroll.buff_type, 0, targetScroll.buff_duration, false);
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

	TargetResult result = ctx.targeting->acquire_targets(ctx, targetScroll.target_mode, wearer.position, targetScroll.range, targetScroll.range);

	if (!result.success)
	{
		ctx.rendering_manager->restore_screen(ctx);
		return false;
	}

	switch (targetScroll.target_mode)
	{
	case TargetMode::AUTO_NEAREST:
	{
		if (!result.creatures.empty())
		{
			auto* target = result.creatures[0];
			ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, "A lightning bolt strikes the ");
			ctx.messageSystem->append_message_part(WHITE_BLUE_PAIR, target->actorData.name);
			ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, " with a loud thunder!");
			ctx.messageSystem->finalize_message();
			SpellAnimations::animate_lightning(wearer.position, target->position, ctx);
			ctx.messageSystem->message(WHITE_RED_PAIR, std::format("The damage is {} hit points.", targetScroll.damage), true);
			target->destructible->take_damage(*target, targetScroll.damage, ctx);
			ctx.creature_manager->cleanup_dead_creatures(*ctx.creatures);
		}
		break;
	}
	case TargetMode::PICK_TILE_AOE:
	{
		ctx.messageSystem->append_message_part(
			WHITE_BLACK_PAIR,
			std::format("The fireball explodes, burning everything within {} tiles!", targetScroll.range));
		ctx.messageSystem->finalize_message();
		SpellAnimations::animate_explosion(result.impact_pos, targetScroll.range, ctx);
		if (ctx.player->get_tile_distance(result.impact_pos) <= targetScroll.range)
		{
			SpellAnimations::animate_creature_hit(ctx.player->position, ctx);
			ctx.player->destructible->take_damage(*ctx.player, targetScroll.damage, ctx);
		}
		for (auto* t : result.creatures)
		{
			if (!t->destructible->is_dead())
			{
				SpellAnimations::animate_creature_hit(t->position, ctx);
				ctx.messageSystem->append_message_part(
					WHITE_BLACK_PAIR,
					std::format("The {} gets engulfed in flames! ({} damage)", t->actorData.name, targetScroll.damage));
				ctx.messageSystem->finalize_message();
			}
		}
		for (auto* t : result.creatures)
		{
			if (!t->destructible->is_dead())
				t->destructible->take_damage(*t, targetScroll.damage, ctx);
		}
		ctx.creature_manager->cleanup_dead_creatures(*ctx.creatures);
		break;
	}
	case TargetMode::PICK_TILE_SINGLE:
	{
		if (!result.creatures.empty())
		{
			auto* target = result.creatures[0];
			target->apply_confusion(targetScroll.confuse_turns);
			ctx.messageSystem->message(
				WHITE_BLACK_PAIR,
				std::format("The eyes of the {} look vacant, as he starts to stumble around!", target->actorData.name),
				true);
		}
		break;
	}
	case TargetMode::FOV_BUFF:
		break;
	}

	ctx.rendering_manager->restore_screen(ctx);
	return consume_item(owner, wearer);
}

bool use(Teleporter& t, Item& owner, Creature& wearer, GameContext& ctx)
{
	(void)t;
	const Vector2D loc = find_valid_teleport_location(ctx);

	if (loc.x != -1 && loc.y != -1)
	{
		wearer.position = loc;
		ctx.map->compute_fov(ctx);
		ctx.messageSystem->message(BLUE_BLACK_PAIR, "You feel disoriented as the world shifts around you!", true);
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "You have been teleported to a new location.", true);
		return consume_item(owner, wearer);
	}

	ctx.messageSystem->message(RED_BLACK_PAIR, "The teleportation magic fizzles out - no safe location found!", true);
	return false;
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
	ctx.hunger_system->decrease_hunger(ctx, f.nutrition_value);
	ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, "You eat the ");
	ctx.messageSystem->append_message_part(YELLOW_BLACK_PAIR, owner.actorData.name);
	ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, ".");
	ctx.messageSystem->finalize_message();
	return consume_item(owner, wearer);
}

bool use(CorpseFood& cf, Item& owner, Creature& wearer, GameContext& ctx)
{
	if (cf.nutrition_value <= 0)
		cf.nutrition_value = get_or_default(CORPSE_NUTRITION_VALUES, owner.actorData.name, 50);

	int actual = cf.nutrition_value + ctx.dice->roll(-10, 10);
	actual = std::max(10, actual);

	ctx.hunger_system->decrease_hunger(ctx, actual);

	const std::string& flavor =
		get_or_default(CORPSE_FLAVOR_TEXT, owner.actorData.name, std::string{ "It tastes... questionable." });

	ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, "You eat the ");
	ctx.messageSystem->append_message_part(RED_BLACK_PAIR, owner.actorData.name);
	ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, ". " + flavor);
	ctx.messageSystem->finalize_message();
	return consume_item(owner, wearer);
}

bool use(Armor& a, Item& item, Creature& wearer, GameContext& ctx)
{
	(void)a;
	// Player path
	if (wearer.uniqueId == ctx.player->uniqueId)
	{
		const bool was_equipped = wearer.is_item_equipped(item.uniqueId);

		// Player* cast is safe: we just verified uniqueId matches player
		auto* player = static_cast<Player*>(&wearer);
		const bool success = player->toggle_armor(item.uniqueId, ctx);

		if (success)
		{
			if (was_equipped)
				ctx.messageSystem->message(WHITE_BLACK_PAIR, "You remove the " + item.actorData.name + ".", true);
			else
				ctx.messageSystem->message(WHITE_BLACK_PAIR, "You put on the " + item.actorData.name + ".", true);
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

bool use(Amulet& a, Item& owner, Creature& wearer, GameContext& ctx)
{
	(void)a;
	(void)wearer;
	ctx.messageSystem->message(WHITE_BLACK_PAIR, "The Amulet of Yendor glows brightly in your hands!", true);
	ctx.messageSystem->message(WHITE_BLACK_PAIR, "You feel a powerful magic enveloping you...", true);
	ctx.game_state->set_game_status(GameStatus::VICTORY);
	return false;
}

// ========== get_ac_bonus() implementations ==========

int get_ac_bonus(const Consumable&) noexcept
{
	return 0;
}
int get_ac_bonus(const Weapon&) noexcept
{
	return 0;
}
int get_ac_bonus(const Shield&) noexcept
{
	return -1;
} // +1 AC in AD&D terms
int get_ac_bonus(const TargetedScroll&) noexcept
{
	return 0;
}
int get_ac_bonus(const Teleporter&) noexcept
{
	return 0;
}
int get_ac_bonus(const Gold&) noexcept
{
	return 0;
}
int get_ac_bonus(const Food&) noexcept
{
	return 0;
}
int get_ac_bonus(const CorpseFood&) noexcept
{
	return 0;
}
int get_ac_bonus(const Armor& a) noexcept
{
	return a.armor_class;
}
int get_ac_bonus(const MagicalHelm& mh) noexcept
{
	return MagicalEffectUtils::get_ac_bonus(mh.effect, mh.bonus);
}
int get_ac_bonus(const MagicalRing& mr) noexcept
{
	return MagicalEffectUtils::get_protection_bonus(mr.effect);
}
int get_ac_bonus(const JewelryAmulet&) noexcept
{
	return 0;
}
int get_ac_bonus(const Gauntlets&) noexcept
{
	return 0;
}
int get_ac_bonus(const Girdle&) noexcept
{
	return 0;
}
int get_ac_bonus(const Amulet&) noexcept
{
	return 0;
}

// ========== Variant-level dispatchers ==========

bool use_item(ItemBehavior& behavior, Item& owner, Creature& wearer, GameContext& ctx)
{
	return std::visit(
		[&owner, &wearer, &ctx](auto& b)
		{ return use(b, owner, wearer, ctx); },
		behavior);
}

int get_item_ac_bonus(const ItemBehavior& behavior) noexcept
{
	return std::visit(
		[](const auto& b)
		{ return get_ac_bonus(b); },
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
				j["buff_type"] = static_cast<int>(b.buff_type);
				j["is_set_effect"] = b.is_set_effect;
			}
			else if constexpr (std::is_same_v<T, Weapon>)
			{
				j["type"] = static_cast<int>(PickableType::WEAPON);
				j["ranged"] = b.ranged;
				j["hand_req"] = static_cast<int>(b.hand_requirement);
				j["weapon_size"] = static_cast<int>(b.weapon_size);
			}
			else if constexpr (std::is_same_v<T, Shield>)
			{
				j["type"] = static_cast<int>(PickableType::SHIELD);
			}
			else if constexpr (std::is_same_v<T, TargetedScroll>)
			{
				j["type"] = static_cast<int>(PickableType::TARGETED_SCROLL);
				j["target_mode"] = static_cast<int>(b.target_mode);
				j["animation"] = static_cast<int>(b.scroll_animation);
				j["range"] = b.range;
				j["damage"] = b.damage;
				j["confuse_turns"] = b.confuse_turns;
				j["buff_type"] = static_cast<int>(b.buff_type);
				j["buff_duration"] = b.buff_duration;
			}
			else if constexpr (std::is_same_v<T, Teleporter>)
			{
				j["type"] = static_cast<int>(PickableType::TELEPORTER);
			}
			else if constexpr (std::is_same_v<T, Gold>)
			{
				j["type"] = static_cast<int>(PickableType::GOLD_COIN);
				j["amount"] = b.amount;
			}
			else if constexpr (std::is_same_v<T, Food>)
			{
				j["type"] = static_cast<int>(PickableType::FOOD);
				j["nutrition_value"] = b.nutrition_value;
			}
			else if constexpr (std::is_same_v<T, CorpseFood>)
			{
				j["type"] = static_cast<int>(PickableType::CORPSE_FOOD);
				j["nutrition_value"] = b.nutrition_value;
			}
			else if constexpr (std::is_same_v<T, Armor>)
			{
				j["type"] = static_cast<int>(PickableType::ARMOR);
				j["armorClass"] = b.armor_class;
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
		},
		behavior);
}

ItemBehavior load_behavior(const json& j)
{
	if (!j.contains("type") || !j["type"].is_number())
		throw std::runtime_error("Invalid JSON format: Missing or invalid 'type'");

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
		if (j.contains("buff_type"))
		{
			c.buff_type = static_cast<BuffType>(j["buff_type"].get<int>());
		}
		if (j.contains("is_set_effect"))
		{
			c.is_set_effect = j["is_set_effect"].get<bool>();
		}

		return c;
	}

	case PickableType::WEAPON:
	{
		Weapon w;
		if (j.contains("ranged"))
			w.ranged = j["ranged"].get<bool>();
		if (j.contains("hand_req"))
			w.hand_requirement = static_cast<HandRequirement>(j["hand_req"].get<int>());
		if (j.contains("weapon_size"))
			w.weapon_size = static_cast<WeaponSize>(j["weapon_size"].get<int>());
		return w;
	}

	case PickableType::SHIELD:
	{
		return Shield{};
	}

	case PickableType::TARGETED_SCROLL:
	{
		TargetedScroll targetedScroll;
		if (j.contains("target_mode"))
		{
			targetedScroll.target_mode = static_cast<TargetMode>(j["target_mode"].get<int>());
		}
		if (j.contains("animation"))
		{
			targetedScroll.scroll_animation = static_cast<ScrollAnimation>(j["animation"].get<int>());
		}
		if (j.contains("range"))
		{
			targetedScroll.range = j["range"].get<int>();
		}
		if (j.contains("damage"))
		{
			targetedScroll.damage = j["damage"].get<int>();
		}
		if (j.contains("confuse_turns"))
		{
			targetedScroll.confuse_turns = j["confuse_turns"].get<int>();
		}
		if (j.contains("buff_type"))
		{
			targetedScroll.buff_type = static_cast<BuffType>(j["buff_type"].get<int>());
		}
		if (j.contains("buff_duration"))
		{
			targetedScroll.buff_duration = j["buff_duration"].get<int>();
		}
		return targetedScroll;
	}

	case PickableType::TELEPORTER:
	{
		return Teleporter{};
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
		if (j.contains("nutrition_value"))
			f.nutrition_value = j["nutrition_value"].get<int>();
		return f;
	}

	case PickableType::CORPSE_FOOD:
	{
		CorpseFood cf;
		if (j.contains("nutrition_value"))
			cf.nutrition_value = j["nutrition_value"].get<int>();
		return cf;
	}

	case PickableType::ARMOR:
	{
		Armor a;
		a.armor_class = j.at("armorClass").get<int>();
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

	default:
	{
		throw std::runtime_error(std::format("Unknown PickableType: {}", static_cast<int>(type)));
	}

	}
}
