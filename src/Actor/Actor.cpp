#include <iostream>
#include <string>
#include <algorithm>
#include <math.h>
#include <memory>
#include <format>

#include <curses.h>
#pragma warning (push, 0)
#include <libtcod.h>
#pragma warning (pop)

#include "../Ai/Ai.h"
#include "Actor.h"
#include "Attacker.h"
#include "Destructible.h"
#include "Pickable.h"
#include "InventoryOperations.h"
#include "../Items/Items.h"
#include "../ActorTypes/Gold.h"
#include "../Items/Armor.h"
#include "../Items/ItemClassification.h"
#include "../Combat/WeaponDamageRegistry.h"
#include "../Systems/ShopKeeper.h"
#include "../Core/GameContext.h"
#include "../Map/Map.h"
#include "../Systems/MessageSystem.h"

using namespace InventoryOperations; // For clean function calls

//====
Actor::Actor(Vector2D position, ActorData data)
	:
	position(position),
	actorData(data),
	uniqueId(UniqueId::Generator::generate())
{}

//====
void Actor::load(const json& j)
{
	position.x = j["position"]["x"];
	position.y = j["position"]["y"];
	direction.x = j["direction"]["x"];
	direction.y = j["direction"]["y"];
	actorData.ch = j["actorData"].at("ch").get<char>();
	actorData.name = j["actorData"].at("name").get<std::string>();
	actorData.color = j["actorData"].at("color").get<int>();
	uniqueId = j.at("uniqueId").get<UniqueId::IdType>();

	// Clear existing states before loading saved states
	states.clear();

	// Deserialize vector of states
	for (const auto& state : j["states"])
	{
		states.push_back(state);
	}
}

void Actor::save(json& j)
{
	j["position"] = { {"y", position.y}, {"x", position.x} };
	j["direction"] = { {"y", direction.y}, {"x", direction.x} };
	j["actorData"] = {
		{"ch", actorData.ch},
		{"name", actorData.name},
		{"color", actorData.color}
	};
	j["uniqueId"] = uniqueId;

	// Serialize vector of states
	json statesJson;
	for (const auto& state : states)
	{
		statesJson.push_back(state);
	}
	j["states"] = statesJson;
}

// a function to get the Chebyshev distance from an actor to a specific tile of the map
int Actor::get_tile_distance(Vector2D tilePosition) const noexcept
{
	return std::max(abs(position.x - tilePosition.x), abs(position.y - tilePosition.y));
}

// the actor render function with color
void Actor::render(const GameContext& ctx) const noexcept
{
	if (is_visible(ctx))
	{
		attron(COLOR_PAIR(actorData.color));
		mvaddch(position.y, position.x, actorData.ch);
		attroff(COLOR_PAIR(actorData.color));
	}
}

// check if the actor is visible
bool Actor::is_visible(const GameContext& ctx) const noexcept
{
	return (!has_state(ActorState::FOV_ONLY) && ctx.map->is_explored(position)) || ctx.map->is_in_fov(position);
}

//==Creature==
void Creature::load(const json& j)
{
	Actor::load(j); // Call base class load
	strength = j["strength"];
	dexterity = j["dexterity"];
	constitution = j["constitution"];
	intelligence = j["intelligence"];
	wisdom = j["wisdom"];
	charisma = j["charisma"];
	playerLevel = j["playerLevel"];
	gold = j["gold"];
	gender = j["gender"];
	weaponEquipped = j["weaponEquipped"];
	if (j.contains("attacker"))
	{
		attacker = std::make_unique<Attacker>(DamageInfo{});
		attacker->load(j["attacker"]);
	}
	if (j.contains("destructible"))
	{
		destructible = Destructible::create(j["destructible"]);
	}
	if (j.contains("ai"))
	{
		ai = Ai::create(j["ai"]);
	}
	if (j.contains("inventory_data"))
	{
		inventory_data = InventoryData(50); // Default capacity
		load_inventory(inventory_data, j["inventory_data"]);
	}
	if (j.contains("shop"))
	{
		shop = ShopKeeper::create(j["shop"]);
	}
}

void Creature::save(json& j)
{
	Actor::save(j); // Call base class save
	j["strength"] = strength;
	j["dexterity"] = dexterity;
	j["constitution"] = constitution;
	j["intelligence"] = intelligence;
	j["wisdom"] = wisdom;
	j["charisma"] = charisma;
	j["playerLevel"] = playerLevel;
	j["gold"] = gold;
	j["gender"] = gender;
	j["weaponEquipped"] = weaponEquipped;
	if (attacker)
	{
		json attackerJson;
		attacker->save(attackerJson);
		j["attacker"] = attackerJson;
	}
	if (destructible)
	{
		json destructibleJson;
		destructible->save(destructibleJson);
		j["destructible"] = destructibleJson;
	}
	if (ai)
	{
		json aiJson;
		ai->save(aiJson);
		j["ai"] = aiJson;
	}
	// Always save inventory data since it always exists
	json inventoryJson;
	save_inventory(inventory_data, inventoryJson);
	j["inventory_data"] = inventoryJson;
	if (shop)
	{
		json shopJson;
		shop->save(shopJson);
		j["shop"] = shopJson;
	}
}

// the actor update
void Creature::update(GameContext& ctx)
{
	// Apply the modifiers from stats and items
	if (destructible)
	{
		destructible->update_armor_class(*this, ctx);
		destructible->update_constitution_bonus(*this, ctx);
	}

	// if the actor has an ai then update the ai
	if (ai)
	{
		ai->update(*this, ctx);
	}
}

void Creature::equip(Item& item, GameContext& ctx)
{
	bool isArmor = item.is_armor();
	bool isWeapon = item.is_weapon();
	bool isShield = item.is_shield();

	// First check if any equipment of the same type is already equipped
	std::vector<Item*> equippedItems;

	// Find all equipped items
	for (const auto& inv_item : inventory_data.items)
	{
		if (inv_item && inv_item->has_state(ActorState::IS_EQUIPPED))
		{
			bool itemIsArmor = inv_item->is_armor();
			bool itemIsWeapon = inv_item->is_weapon();
			bool itemIsShield = inv_item->is_shield();

			// Only consider same-type equipment for unequipping
			if ((isArmor && itemIsArmor) || (isWeapon && itemIsWeapon) || (isShield && itemIsShield))
			{
				equippedItems.push_back(inv_item.get());
			}
		}
	}

	// If there's already equipment of the same type, unequip it
	if (!equippedItems.empty() && &item != equippedItems[0])
	{
		for (auto* equipped : equippedItems)
		{
			unequip(*equipped, ctx);
		}
	}

	// Now equip the new item
	item.add_state(ActorState::IS_EQUIPPED);

	// Update weapon equipped name and damage if it's a weapon
	if (isWeapon)
	{
		weaponEquipped = ItemClassificationUtils::get_display_name(item.itemClass);
		std::string weaponDamage = WeaponDamageRegistry::get_damage_roll(item.itemClass);
		ctx.message_system->log(std::format("Equipped {} - damage: {}", ItemClassificationUtils::get_display_name(item.itemClass), weaponDamage));
	}

	// Log shield equipped
	if (isShield)
	{
		ctx.message_system->log(std::format("Equipped {} shield", ItemClassificationUtils::get_display_name(item.itemClass)));
	}

	// Log armor equipped
	if (isArmor)
	{
		ctx.message_system->log(std::format("Equipped {} armor", ItemClassificationUtils::get_display_name(item.itemClass)));
	}
}

void Creature::unequip(Item& item, GameContext& ctx)
{
	// Check if the item is actually equipped
	if (item.has_state(ActorState::IS_EQUIPPED))
	{
		// Remove the equipped state
		item.remove_state(ActorState::IS_EQUIPPED);

		// If it's a weapon, update the weaponEquipped status
		if (item.is_weapon())
		{
			weaponEquipped = "None";
			ctx.message_system->log("Unequipped weapon - now unarmed");

			// Check for ranged weapon - use ItemClass system
			if (item.is_ranged_weapon())
			{
				// Remove the ranged state
				if (has_state(ActorState::IS_RANGED))
				{
					remove_state(ActorState::IS_RANGED);
					ctx.message_system->log("Removed IS_RANGED state after unequipping " + item.actorData.name);
				}
			}
		}

		// Double-check all inventory to see if we still should have IS_RANGED
		bool hasRangedWeapon = false;
		for (const auto& invItem : inventory_data.items)
		{
			if (invItem && invItem->has_state(ActorState::IS_EQUIPPED) && invItem->pickable)
			{
				if (invItem->is_ranged_weapon())
				{
					hasRangedWeapon = true;
					break;
				}
			}
		}

		// Force sync the IS_RANGED state
		if (!hasRangedWeapon && has_state(ActorState::IS_RANGED))
		{
			remove_state(ActorState::IS_RANGED);
			ctx.message_system->log("Force removed IS_RANGED state - no ranged weapons equipped");
		}
	}
}

void Creature::sync_ranged_state(GameContext& ctx)
{
	// Check if any equipped items are ranged weapons
	bool hasRangedWeapon = false;
	for (const auto& item : inventory_data.items)
	{
		if (item && item->has_state(ActorState::IS_EQUIPPED) && item->pickable)
		{
			if (item->is_ranged_weapon())
			{
				hasRangedWeapon = true;
				break;
			}
		}
	}

	// Make sure IS_RANGED state matches equipped weapons
	if (hasRangedWeapon && !has_state(ActorState::IS_RANGED))
	{
		add_state(ActorState::IS_RANGED);
		ctx.message_system->log("Added missing IS_RANGED state - ranged weapon equipped");
	}
	else if (!hasRangedWeapon && has_state(ActorState::IS_RANGED))
	{
		remove_state(ActorState::IS_RANGED);
		ctx.message_system->log("Removed incorrect IS_RANGED state - no ranged weapons equipped");
	}
}

void Creature::pick(GameContext& ctx)
{
	// Check if inventory is already full before attempting to pick
	if (is_inventory_full(inventory_data))
	{
		ctx.message_system->message(WHITE_BLACK_PAIR, "Your inventory is full! You can't carry any more items.", true);
		return;
	}

	// Find item at player's position using proper inventory interface
	Item* itemAtPosition = nullptr;
	size_t itemIndex = 0;

	if (ctx.inventory_data->items.empty())
	{
		return; // No floor items
	}
	
	// Search for item at current position
	for (size_t i = 0; i < ctx.inventory_data->items.size(); ++i)
	{
		if (auto& item = ctx.inventory_data->items[i])
		{
			if (position == item->position)
			{
				itemAtPosition = item.get();
				itemIndex = i;
				break;
			}
		}
	}

	if (!itemAtPosition)
	{
		return; // No item at this position
	}

	// Handle item pickup using proper inventory operations
	if (itemAtPosition->itemClass == ItemClass::GOLD)
	{
		// Use the pickable's use() method which handles gold pickup properly
		if (itemAtPosition->pickable->use(*itemAtPosition, *this, ctx))
		{
			// Gold was picked up successfully via polymorphic call
			// Remove gold from floor inventory
			auto removeResult = remove_item_at(*ctx.inventory_data, itemIndex);
			if (!removeResult.has_value())
			{
				ctx.message_system->log("WARNING: Failed to remove gold item from floor inventory");
			}
		}
	}
	else
	{
		// Normal item handling - store name before moving
		const std::string itemName = itemAtPosition->actorData.name;

		// Remove from floor first
		auto removeResult = remove_item_at(*ctx.inventory_data, itemIndex);
		if (removeResult.has_value())
		{
			// Add to player inventory
			auto addResult = add_item(inventory_data, std::move(*removeResult));
			if (addResult.has_value())
			{
				ctx.message_system->message(WHITE_BLACK_PAIR, "You picked up the " + itemName + ".", true);
			}
			else
			{
				ctx.message_system->log("WARNING: Failed to add item to player inventory");
				// Item is lost - this should not happen since we checked is_full() earlier
			}
		}
		else
		{
			ctx.message_system->log("WARNING: Failed to remove item from floor inventory");
		}
	}
}

void Creature::drop(Item& item, GameContext& ctx)
{
	// Check if the item is actually in the inventory first
	auto it = std::find_if(inventory_data.items.begin(), inventory_data.items.end(),
		[&item](const auto& invItem) { return invItem.get() == &item; });

	if (it != inventory_data.items.end())
	{
		// Set the item's position to the player's position
		(*it)->position = position;

		// If the item is equipped, unequip it first
		if ((*it)->has_state(ActorState::IS_EQUIPPED))
		{
			unequip(*(*it), ctx);
		}

		// Add to game floor inventory
		auto addResult = add_item(*ctx.inventory_data, std::move(*it));
		if (addResult.has_value())
		{
			// Clean up null pointer that remains after moving
			optimize_inventory_storage(inventory_data);
			ctx.message_system->message(WHITE_BLACK_PAIR, "You dropped the item.", true);
		}
	}
}

//==Item==
Item::Item(Vector2D position, ActorData data) : Object(position, data)
{
	// Initialize item type from name when created
	initialize_item_type_from_name();
};

void Item::load(const json& j)
{
	Object::load(j); // Call base class load
	value = j.at("value").get<int>();
	base_value = j.at("base_value").get<int>();
	itemClass = static_cast<ItemClass>(j.at("itemClass").get<int>());

	// Load enhancement data
	if (j.contains("enhancement"))
	{
		const auto& enh = j["enhancement"];
		enhancement.prefix = static_cast<PrefixType>(enh.at("prefix").get<int>());
		enhancement.suffix = static_cast<SuffixType>(enh.at("suffix").get<int>());
		enhancement.damage_bonus = enh.at("damage_bonus").get<int>();
		enhancement.to_hit_bonus = enh.at("to_hit_bonus").get<int>();
		enhancement.ac_bonus = enh.at("ac_bonus").get<int>();
		enhancement.strength_bonus = enh.at("strength_bonus").get<int>();
		enhancement.dexterity_bonus = enh.at("dexterity_bonus").get<int>();
		enhancement.intelligence_bonus = enh.at("intelligence_bonus").get<int>();
		enhancement.hp_bonus = enh.at("hp_bonus").get<int>();
		enhancement.mana_bonus = enh.at("mana_bonus").get<int>();
		enhancement.speed_bonus = enh.at("speed_bonus").get<int>();
		enhancement.stealth_bonus = enh.at("stealth_bonus").get<int>();
		enhancement.fire_resistance = enh.at("fire_resistance").get<int>();
		enhancement.cold_resistance = enh.at("cold_resistance").get<int>();
		enhancement.lightning_resistance = enh.at("lightning_resistance").get<int>();
		enhancement.poison_resistance = enh.at("poison_resistance").get<int>();
		enhancement.is_cursed = enh.at("is_cursed").get<bool>();
		enhancement.is_blessed = enh.at("is_blessed").get<bool>();
		enhancement.is_magical = enh.at("is_magical").get<bool>();
		enhancement.enhancement_level = enh.at("enhancement_level").get<int>();
		enhancement.value_modifier = enh.at("value_modifier").get<int>();
	}

	if (j.contains("pickable")) {
		pickable = Pickable::create(j["pickable"]);
	}
}

void Item::save(json& j)
{
	Object::save(j); // Call base class save
	j["value"] = value;
	j["base_value"] = base_value;
	j["itemClass"] = static_cast<int>(itemClass);

	// Save enhancement data
	json enh;
	enh["prefix"] = static_cast<int>(enhancement.prefix);
	enh["suffix"] = static_cast<int>(enhancement.suffix);
	enh["damage_bonus"] = enhancement.damage_bonus;
	enh["to_hit_bonus"] = enhancement.to_hit_bonus;
	enh["ac_bonus"] = enhancement.ac_bonus;
	enh["strength_bonus"] = enhancement.strength_bonus;
	enh["dexterity_bonus"] = enhancement.dexterity_bonus;
	enh["intelligence_bonus"] = enhancement.intelligence_bonus;
	enh["hp_bonus"] = enhancement.hp_bonus;
	enh["mana_bonus"] = enhancement.mana_bonus;
	enh["speed_bonus"] = enhancement.speed_bonus;
	enh["stealth_bonus"] = enhancement.stealth_bonus;
	enh["fire_resistance"] = enhancement.fire_resistance;
	enh["cold_resistance"] = enhancement.cold_resistance;
	enh["lightning_resistance"] = enhancement.lightning_resistance;
	enh["poison_resistance"] = enhancement.poison_resistance;
	enh["is_cursed"] = enhancement.is_cursed;
	enh["is_blessed"] = enhancement.is_blessed;
	enh["is_magical"] = enhancement.is_magical;
	enh["enhancement_level"] = enhancement.enhancement_level;
	enh["value_modifier"] = enhancement.value_modifier;
	j["enhancement"] = enh;

	if (pickable) {
		json pickableJson;
		pickable->save(pickableJson);
		j["pickable"] = pickableJson;
	}
}

void Item::initialize_item_type_from_name()
{
	// Temporary bridge: Set ItemClass based on item name
	// This should be replaced when item creation system is properly refactored
	std::string name = actorData.name;
	
	// Convert to lowercase for easier matching
	std::transform(name.begin(), name.end(), name.begin(), ::tolower);
	
	// Weapons - Melee
	if (name.find("dagger") != std::string::npos) itemClass = ItemClass::DAGGER;
	else if (name.find("short sword") != std::string::npos) itemClass = ItemClass::SHORT_SWORD;
	else if (name.find("long sword") != std::string::npos) itemClass = ItemClass::LONG_SWORD;
	else if (name.find("great sword") != std::string::npos) itemClass = ItemClass::GREAT_SWORD;
	else if (name.find("battle axe") != std::string::npos) itemClass = ItemClass::BATTLE_AXE;
	else if (name.find("great axe") != std::string::npos) itemClass = ItemClass::GREAT_AXE;
	else if (name.find("war hammer") != std::string::npos) itemClass = ItemClass::WAR_HAMMER;
	else if (name.find("staff") != std::string::npos) itemClass = ItemClass::STAFF;
	
	// Weapons - Ranged
	else if (name.find("long bow") != std::string::npos) itemClass = ItemClass::LONG_BOW;
	else if (name.find("bow") != std::string::npos) itemClass = ItemClass::LONG_BOW; // Default bow type
	
	// Armor
	else if (name.find("leather armor") != std::string::npos) itemClass = ItemClass::LEATHER_ARMOR;
	else if (name.find("chain mail") != std::string::npos) itemClass = ItemClass::CHAIN_MAIL;
	else if (name.find("plate mail") != std::string::npos) itemClass = ItemClass::PLATE_MAIL;
	
	// Shields
	else if (name.find("large shield") != std::string::npos) itemClass = ItemClass::LARGE_SHIELD;
	else if (name.find("small shield") != std::string::npos) itemClass = ItemClass::SMALL_SHIELD;
	else if (name.find("shield") != std::string::npos) itemClass = ItemClass::MEDIUM_SHIELD;
	
	// Consumables
	else if (name.find("health potion") != std::string::npos) itemClass = ItemClass::HEALTH_POTION;
	else if (name.find("potion") != std::string::npos) itemClass = ItemClass::HEALTH_POTION; // Default potion
	else if (name.find("bread") != std::string::npos) itemClass = ItemClass::BREAD;
	else if (name.find("meat") != std::string::npos) itemClass = ItemClass::MEAT;
	else if (name.find("fruit") != std::string::npos) itemClass = ItemClass::FRUIT;
	else if (name.find("food") != std::string::npos) itemClass = ItemClass::FOOD_RATION;
	
	// Scrolls
	else if (name.find("lightning") != std::string::npos) itemClass = ItemClass::SCROLL_LIGHTNING;
	else if (name.find("fireball") != std::string::npos) itemClass = ItemClass::SCROLL_FIREBALL;
	else if (name.find("confusion") != std::string::npos) itemClass = ItemClass::SCROLL_CONFUSION;
	
	// Treasure
	else if (name.find("gold") != std::string::npos) itemClass = ItemClass::GOLD;
	else if (name.find("amulet") != std::string::npos) itemClass = ItemClass::AMULET;
	
	// Default to unknown if no match found
	else itemClass = ItemClass::UNKNOWN;
}

const std::string& Item::get_name() const noexcept
{
	if (is_enhanced())
	{
		static std::string enhanced_name;
		enhanced_name = enhancement.get_full_name(actorData.name);
		return enhanced_name;
	}
	return actorData.name;
}

void Item::apply_enhancement(const ItemEnhancement& new_enhancement)
{
	enhancement = new_enhancement;
	// Update value based on enhancement
	value = (base_value * enhancement.value_modifier) / 100;
}

void Item::generate_random_enhancement(bool allow_magical)
{
	if (is_weapon())
	{
		enhancement = ItemEnhancement::generate_weapon_enhancement();
	}
	else if (is_armor())
	{
		enhancement = ItemEnhancement::generate_armor_enhancement();
	}
	else
	{
		enhancement = ItemEnhancement::generate_random_enhancement(allow_magical);
	}
	
	// Update value based on enhancement
	value = (base_value * enhancement.value_modifier) / 100;
}

bool Item::is_enhanced() const noexcept
{
	return enhancement.prefix != PrefixType::NONE || enhancement.suffix != SuffixType::NONE;
}

int Item::get_enhanced_value() const noexcept
{
	return (base_value * enhancement.value_modifier) / 100;
}

// end of file: Actor.cpp
