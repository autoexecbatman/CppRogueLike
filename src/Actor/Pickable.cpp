// file: Pickable.cpp
#include <format>

#include "Pickable.h"
#include "../Colors/Colors.h"
#include "Actor.h"
#include "InventoryData.h"
#include "InventoryOperations.h"
#include "../ActorTypes/Player.h"
#include "../ActorTypes/Teleporter.h"
#include "../Systems/SpellAnimations.h"
#include "../Systems/TargetingSystem.h"
#include "../Systems/CreatureManager.h"
#include "../Systems/RenderingManager.h"
#include "../ActorTypes/Gold.h"
#include "../Items/Food.h"
#include "../Items/CorpseFood.h"
#include "../Items/Amulet.h"
#include "../Items/Armor.h"
#include "../Items/Jewelry.h"
#include "../Core/GameContext.h"
#include "../Systems/MessageSystem.h"
#include "../Systems/BuffSystem.h"

using namespace InventoryOperations;

//==PICKABLE==
bool Pickable::use(Item& owner, Creature& wearer, GameContext& ctx)
{
	auto result = remove_item(wearer.inventory_data, owner);
	return result.has_value();
}

std::unique_ptr<Pickable> Pickable::create(const json& j)
{
	if (!j.contains("type") || !j["type"].is_number())
	{
		throw std::runtime_error("Invalid JSON format: Missing or invalid 'type'");
	}

	auto type = static_cast<PickableType>(j["type"].get<int>());
	std::unique_ptr<Pickable> pickable;

	switch (type)
	{
	case PickableType::TARGETED_SCROLL:
		pickable = std::make_unique<TargetedScroll>(TargetMode::AUTO_NEAREST, ScrollAnimation::NONE, 0, 0, 0);
		break;
	case PickableType::TELEPORTER:
		pickable = std::make_unique<Teleporter>();
		break;
	case PickableType::WEAPON:
		// Defaults overwritten by load() below
		pickable = std::make_unique<Weapon>(false, HandRequirement::ONE_HANDED, WeaponSize::MEDIUM);
		break;
	case PickableType::SHIELD:
		pickable = std::make_unique<Shield>();
		break;
	case PickableType::CONSUMABLE:
		// Defaults overwritten by load() below
		pickable = std::make_unique<Consumable>(ConsumableEffect::NONE, 0, 0, BuffType::INVISIBILITY);
		break;
	case PickableType::GOLD:
		pickable = std::make_unique<Gold>(0);
		break;
	case PickableType::FOOD:
		pickable = std::make_unique<Food>(0);
		break;
	case PickableType::CORPSE_FOOD:
		pickable = std::make_unique<CorpseFood>(0);
		break;
	case PickableType::ARMOR:
		pickable = std::make_unique<Armor>(0);
		break;
	case PickableType::MAGICAL_HELM:
		pickable = std::make_unique<MagicalHelm>(MagicalEffect::NONE, 0);
		break;
	case PickableType::MAGICAL_RING:
		pickable = std::make_unique<MagicalRing>(MagicalEffect::NONE, 0);
		break;
	case PickableType::JEWELRY_AMULET:
		pickable = std::make_unique<JewelryAmulet>();
		break;
	case PickableType::GAUNTLETS:
		pickable = std::make_unique<Gauntlets>();
		break;
	case PickableType::GIRDLE:
		pickable = std::make_unique<Girdle>();
		break;
	case PickableType::QUEST_ITEM:
		pickable = std::make_unique<Amulet>();
		break;
	default:
		throw std::runtime_error(std::format("Unknown PickableType: {}", static_cast<int>(type)));
	}

	pickable->load(j);
	return pickable;
}

//==CONSUMABLE==
bool Consumable::use(Item& owner, Creature& wearer, GameContext& ctx)
{
	switch (effect)
	{
	case ConsumableEffect::HEAL:
	{
		if (!wearer.destructible)
			return false;
		if (wearer.destructible->get_hp() >= wearer.destructible->get_max_hp())
		{
			ctx.message_system->message(WHITE_BLACK_PAIR, "You are already at full health.", true);
			return false;
		}
		const int healed = wearer.destructible->heal(amount);
		ctx.message_system->message(GREEN_BLACK_PAIR, std::format("You feel better! (+{} HP)", healed), true);
		break;
	}
	case ConsumableEffect::ADD_BUFF:
		ctx.buff_system->add_buff(wearer, buff_type, amount, duration, is_set_effect);
		ctx.message_system->message(CYAN_BLACK_PAIR, std::format("You feel the effect of the {} for {} turns.", owner.get_name(), duration), true);
		break;
	case ConsumableEffect::NONE:
		ctx.message_system->message(WHITE_BLACK_PAIR, std::format("You use the {}.", owner.get_name()), true);
		break;
	case ConsumableEffect::FAIL:
		ctx.message_system->message(RED_BLACK_PAIR, std::format("Nothing happens with the {}.", owner.get_name()), true);
		return false;
	}

	auto result = InventoryOperations::remove_item(wearer.inventory_data, owner);
	return result.has_value();
}

void Consumable::save(json& j)
{
	j["type"] = static_cast<int>(PickableType::CONSUMABLE);
	j["effect"] = static_cast<int>(effect);
	j["amount"] = amount;
	j["duration"] = duration;
	j["buff_type"] = static_cast<int>(buff_type);
	j["is_set_effect"] = is_set_effect;
}

void Consumable::load(const json& j)
{
	if (j.contains("effect")) effect = static_cast<ConsumableEffect>(j["effect"].get<int>());
	if (j.contains("amount")) amount = j["amount"].get<int>();
	if (j.contains("duration")) duration = j["duration"].get<int>();
	if (j.contains("buff_type")) buff_type = static_cast<BuffType>(j["buff_type"].get<int>());
	if (j.contains("is_set_effect")) is_set_effect = j["is_set_effect"].get<bool>();
}

//==WEAPON==
bool Weapon::use(Item& owner, Creature& wearer, GameContext& ctx)
{
	EquipmentSlot preferredSlot = get_preferred_slot(&wearer);
	bool success = wearer.toggle_weapon(owner.uniqueId, preferredSlot, ctx);

	if (success)
	{
		Item* equippedWeapon = wearer.get_equipped_item(preferredSlot);
		if (equippedWeapon && equippedWeapon->uniqueId == owner.uniqueId)
		{
			std::string slotName = (preferredSlot == EquipmentSlot::LEFT_HAND) ? "off-hand" : "main hand";
			ctx.message_system->message(WHITE_BLACK_PAIR, std::format("You equip the {} in your {}.", owner.get_name(), slotName), true);
		}
		else
		{
			ctx.message_system->message(WHITE_BLACK_PAIR, std::format("You unequip the {}.", owner.get_name()), true);
		}
		return true;
	}

	wearer.equip(owner, ctx);
	return true;
}

//==TARGETED_SCROLL==
bool TargetedScroll::use(Item& owner, Creature& wearer, GameContext& ctx)
{
    TargetResult result = ctx.targeting->acquire_targets(ctx, target_mode, wearer.position, range, range);

    if (!result.success)
    {
        ctx.rendering_manager->restore_screen(ctx);
        return false;
    }

    switch (target_mode)
    {
    case TargetMode::AUTO_NEAREST:
    {
        if (!result.creatures.empty())
        {
            auto* target = result.creatures[0];
            ctx.message_system->append_message_part(WHITE_BLACK_PAIR, "A lightning bolt strikes the ");
            ctx.message_system->append_message_part(WHITE_BLUE_PAIR, target->actorData.name);
            ctx.message_system->append_message_part(WHITE_BLACK_PAIR, " with a loud thunder!");
            ctx.message_system->finalize_message();
            SpellAnimations::animate_lightning(wearer.position, target->position, ctx);
            ctx.message_system->message(WHITE_RED_PAIR, std::format("The damage is {} hit points.", damage), true);
            target->destructible->take_damage(*target, damage, ctx);
            ctx.creature_manager->cleanup_dead_creatures(*ctx.creatures);
        }
        break;
    }
    case TargetMode::PICK_TILE_AOE:
    {
        ctx.message_system->append_message_part(WHITE_BLACK_PAIR, std::format("The fireball explodes, burning everything within {} tiles!", range));
        ctx.message_system->finalize_message();
        SpellAnimations::animate_explosion(result.impact_pos, range, ctx);
        if (ctx.player->get_tile_distance(result.impact_pos) <= range)
        {
            SpellAnimations::animate_creature_hit(ctx.player->position, ctx);
            ctx.player->destructible->take_damage(*ctx.player, damage, ctx);
        }
        for (auto* t : result.creatures)
        {
            if (!t->destructible->is_dead())
            {
                SpellAnimations::animate_creature_hit(t->position, ctx);
                ctx.message_system->append_message_part(WHITE_BLACK_PAIR, std::format("The {} gets engulfed in flames! ({} damage)", t->actorData.name, damage));
                ctx.message_system->finalize_message();
            }
        }
        for (auto* t : result.creatures)
        {
            if (!t->destructible->is_dead())
                t->destructible->take_damage(*t, damage, ctx);
        }
        ctx.creature_manager->cleanup_dead_creatures(*ctx.creatures);
        break;
    }
    case TargetMode::PICK_TILE_SINGLE:
    {
        if (!result.creatures.empty())
        {
            auto* target = result.creatures[0];
            target->apply_confusion(confuse_turns);
            ctx.message_system->message(WHITE_BLACK_PAIR,
                std::format("The eyes of the {} look vacant, as he starts to stumble around!", target->actorData.name), true);
        }
        break;
    }
    }

    ctx.rendering_manager->restore_screen(ctx);
    return Pickable::use(owner, wearer, ctx);
}

void TargetedScroll::save(json& j)
{
    j["type"] = static_cast<int>(PickableType::TARGETED_SCROLL);
    j["target_mode"] = static_cast<int>(target_mode);
    j["animation"] = static_cast<int>(scroll_animation);
    j["range"] = range;
    j["damage"] = damage;
    j["confuse_turns"] = confuse_turns;
}

void TargetedScroll::load(const json& j)
{
    if (j.contains("target_mode")) target_mode = static_cast<TargetMode>(j["target_mode"].get<int>());
    if (j.contains("animation")) scroll_animation = static_cast<ScrollAnimation>(j["animation"].get<int>());
    if (j.contains("range")) range = j["range"].get<int>();
    if (j.contains("damage")) damage = j["damage"].get<int>();
    if (j.contains("confuse_turns")) confuse_turns = j["confuse_turns"].get<int>();
}
EquipmentSlot Weapon::get_preferred_slot(const Creature* creature) const
{
	if (ranged)
		return EquipmentSlot::MISSILE_WEAPON;

	// AD&D 2e: small or smaller weapons can go off-hand when main hand holds a larger weapon
	if (!can_be_off_hand(weapon_size))
		return EquipmentSlot::RIGHT_HAND;

	Item* mainHandWeapon = creature->get_equipped_item(EquipmentSlot::RIGHT_HAND);
	Item* offHandItem = creature->get_equipped_item(EquipmentSlot::LEFT_HAND);

	if (!mainHandWeapon || offHandItem)
		return EquipmentSlot::RIGHT_HAND;

	if (mainHandWeapon->is_weapon())
	{
		if (Weapon* mainWeapon = dynamic_cast<Weapon*>(mainHandWeapon->pickable.get()))
		{
			if (mainWeapon->get_weapon_size() > weapon_size)
				return EquipmentSlot::LEFT_HAND;
		}
	}

	return EquipmentSlot::RIGHT_HAND;
}

void Weapon::save(json& j)
{
	j["type"] = static_cast<int>(PickableType::WEAPON);
	j["ranged"] = ranged;
	j["hand_req"] = static_cast<int>(hand_requirement);
	j["weapon_size"] = static_cast<int>(weapon_size);
}

void Weapon::load(const json& j)
{
	if (j.contains("ranged")) ranged = j["ranged"].get<bool>();
	if (j.contains("hand_req")) hand_requirement = static_cast<HandRequirement>(j["hand_req"].get<int>());
	if (j.contains("weapon_size")) weapon_size = static_cast<WeaponSize>(j["weapon_size"].get<int>());
}

bool Weapon::can_be_off_hand(WeaponSize weaponSize) const
{
	return weaponSize <= WeaponSize::SMALL;
}

bool Weapon::validate_dual_wield(Item* mainHandWeapon, Item* offHandWeapon) const
{
	if (!mainHandWeapon || !offHandWeapon)
		return false;
	if (!mainHandWeapon->is_weapon() || !offHandWeapon->is_weapon())
		return false;
	return true;
}

//==SHIELD==
bool Shield::use(Item& owner, Creature& wearer, GameContext& ctx)
{
	bool success = wearer.toggle_shield(owner.uniqueId, ctx);

	if (success)
	{
		Item* equippedShield = wearer.get_equipped_item(EquipmentSlot::LEFT_HAND);
		if (equippedShield && equippedShield->uniqueId == owner.uniqueId)
			ctx.message_system->message(WHITE_BLACK_PAIR, std::format("You raise the {}.", owner.get_name()), true);
		else
			ctx.message_system->message(WHITE_BLACK_PAIR, std::format("You lower the {}.", owner.get_name()), true);
		return true;
	}

	wearer.equip(owner, ctx);
	return true;
}

