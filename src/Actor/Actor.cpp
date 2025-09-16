// file: Actor.cpp
#include <iostream>
#include <string>
#include <algorithm>
#include <math.h>
#include <memory>

#include <curses.h>
#pragma warning (push, 0)
#include <libtcod.h>
#pragma warning (pop)

#include "../Game.h"
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
	
	// Load unique ID (generate new one if missing for backward compatibility)
	if (j.contains("uniqueId"))
	{
		uniqueId = j["uniqueId"].get<UniqueId::IdType>();
	}
	else
	{
		uniqueId = UniqueId::Generator::generate();
	}

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
void Actor::render() const noexcept
{
	if (is_visible())
	{
		attron(COLOR_PAIR(actorData.color));
		mvaddch(position.y, position.x, actorData.ch);
		attroff(COLOR_PAIR(actorData.color));
	}
}

// check if the actor is visible
bool Actor::is_visible() const noexcept
{
	return (!has_state(ActorState::FOV_ONLY) && game.map.is_explored(position)) || game.map.is_in_fov(position);
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
		attacker = std::make_unique<Attacker>("");
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
}

// the actor update
void Creature::update()
{
	// Apply the modifiers from stats and items
	if (destructible)
	{
		destructible->update_armor_class(*this);
		destructible->update_constitution_bonus(*this);
	}

	// if the actor has an ai then update the ai
	if (ai)
	{
		ai->update(*this);
	}
}

void Creature::equip(Item& item)
{
	bool isArmor = item.is_armor();
	bool isWeapon = item.is_weapon();

	// First check if any equipment of the same type is already equipped
	std::vector<Item*> equippedItems;

	// Find all equipped items
	for (const auto& inv_item : inventory_data.items)
	{
		if (inv_item && inv_item->has_state(ActorState::IS_EQUIPPED))
		{
			bool itemIsArmor = inv_item->is_armor();
			bool itemIsWeapon = inv_item->is_weapon();

			// Only consider same-type equipment for unequipping
			if ((isArmor && itemIsArmor) || (isWeapon && itemIsWeapon))
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
			unequip(*equipped);
		}
	}

	// Now equip the new item
	item.add_state(ActorState::IS_EQUIPPED);

	// Update weapon equipped name and damage if it's a weapon
	if (isWeapon)
	{
		weaponEquipped = ItemClassificationUtils::get_display_name(item.itemClass);
		
		// Update attacker damage roll based on weapon class
		if (attacker)
		{
			std::string weaponDamage = WeaponDamageRegistry::get_damage_roll(item.itemClass);
			
			attacker->set_roll(weaponDamage);
			game.log("Equipped " + ItemClassificationUtils::get_display_name(item.itemClass) + " - damage updated to " + weaponDamage);
		}
	}
}

void Creature::unequip(Item& item)
{
	// Check if the item is actually equipped
	if (item.has_state(ActorState::IS_EQUIPPED))
	{
		// Remove the equipped state
		item.remove_state(ActorState::IS_EQUIPPED);

		// If it's a weapon, update the weaponEquipped status and reset damage
		if (item.is_weapon())
		{
			weaponEquipped = "None";
			
			// Reset attacker damage to unarmed
			if (attacker)
			{
				attacker->set_roll(WeaponDamageRegistry::get_unarmed_damage());
				game.log("Unequipped weapon - damage reset to " + WeaponDamageRegistry::get_unarmed_damage() + " (unarmed)");
			}

			// Check for ranged weapon - use ItemClass system
			if (item.is_ranged_weapon())
			{
				// Remove the ranged state
				if (has_state(ActorState::IS_RANGED))
				{
					remove_state(ActorState::IS_RANGED);
					game.log("Removed IS_RANGED state after unequipping " + item.actorData.name);
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
			game.log("Force removed IS_RANGED state - no ranged weapons equipped");
		}
	}
}

void Creature::sync_ranged_state()
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
		game.log("Added missing IS_RANGED state - ranged weapon equipped");
	}
	else if (!hasRangedWeapon && has_state(ActorState::IS_RANGED))
	{
		remove_state(ActorState::IS_RANGED);
		game.log("Removed incorrect IS_RANGED state - no ranged weapons equipped");
	}
}

void Creature::pick()
{
	// Check if inventory is already full before attempting to pick
	if (is_inventory_full(inventory_data))
	{
		game.message(WHITE_BLACK_PAIR, "Your inventory is full! You can't carry any more items.", true);
		return;
	}

	// Find item at player's position using proper inventory interface
	Item* itemAtPosition = nullptr;
	size_t itemIndex = 0;
	
	if (game.inventory_data.items.empty())
	{
		return; // No floor items
	}
	
	// Search for item at current position
	for (size_t i = 0; i < game.inventory_data.items.size(); ++i)
	{
		if (auto& item = game.inventory_data.items[i])
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
		if (itemAtPosition->pickable->use(*itemAtPosition, *this))
		{
			// Gold was picked up successfully via polymorphic call
			// Remove gold from floor inventory
			auto removeResult = remove_item_at(game.inventory_data, itemIndex);
			if (!removeResult.has_value())
			{
				game.log("WARNING: Failed to remove gold item from floor inventory");
			}
		}
	}
	else
	{
		// Normal item handling - store name before moving
		const std::string itemName = itemAtPosition->actorData.name;
		
		// Remove from floor first
		auto removeResult = remove_item_at(game.inventory_data, itemIndex);
		if (removeResult.has_value())
		{
			// Add to player inventory
			auto addResult = add_item(inventory_data, std::move(*removeResult));
			if (addResult.has_value())
			{
				game.message(WHITE_BLACK_PAIR, "You picked up the " + itemName + ".", true);
			}
			else
			{
				game.log("WARNING: Failed to add item to player inventory");
				// Item is lost - this should not happen since we checked is_full() earlier
			}
		}
		else
		{
			game.log("WARNING: Failed to remove item from floor inventory");
		}
	}
}

void Creature::drop(Item& item)
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
			unequip(*(*it));
		}

		// Add to game floor inventory
		auto addResult = add_item(game.inventory_data, std::move(*it));
		if (addResult.has_value())
		{
			// Clean up null pointer that remains after moving
			optimize_inventory_storage(inventory_data);
			game.message(WHITE_BLACK_PAIR, "You dropped the item.", true);
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
	value = j["value"];
	
	// Load ItemClass if available, otherwise initialize from name
	if (j.contains("itemClass"))
	{
		itemClass = static_cast<ItemClass>(j["itemClass"].get<int>());
	}
	else if (j.contains("itemType")) // Backward compatibility
	{
		itemClass = static_cast<ItemClass>(j["itemType"].get<int>());
	}
	else
	{
		initialize_item_type_from_name();
	}
	
	if (j.contains("pickable")) {
		pickable = Pickable::create(j["pickable"]);
	}
}

void Item::save(json& j)
{
	Object::save(j); // Call base class save
	j["value"] = value;
	j["itemClass"] = static_cast<int>(itemClass);
	
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

// end of file: Actor.cpp
