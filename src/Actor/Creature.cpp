#include <algorithm>
#include <cassert>
#include <format>
#include <memory>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

#include "../Actor/InventoryOperations.h"
#include "../Ai/Ai.h"
#include "../Ai/AiMonsterConfused.h"
#include "../Colors/Colors.h"
#include "../Combat/DamageInfo.h"
#include "../Combat/WeaponDamageRegistry.h"
#include "../Core/GameContext.h"
#include "../Items/ItemClassification.h"
#include "../Persistent/Persistent.h"
#include "../Systems/BuffSystem.h"
#include "../Systems/BuffType.h"
#include "../Systems/MessageSystem.h"
#include "../Systems/ShopKeeper.h"
#include "../Systems/TileConfig.h"
#include "Actor.h"
#include "Attacker.h"
#include "Creature.h"
#include "Destructible.h"
#include "EquipmentSlot.h"
#include "InventoryData.h"
#include "Item.h"
#include "Pickable.h"

//==Creature==
void Creature::load(const json& j)
{
	Actor::load(j); // Call base class load
	baseStrength = j["strength"];
	baseDexterity = j["dexterity"];
	baseConstitution = j["constitution"];
	baseIntelligence = j["intelligence"];
	baseWisdom = j["wisdom"];
	baseCharisma = j["charisma"];
	creatureLevel = j["playerLevel"];
	gold = j["gold"];
	gender = j["gender"];
	weaponEquipped = j["weaponEquipped"];
	creatureClass = static_cast<CreatureClass>(j.value("creatureClass", static_cast<int>(CreatureClass::MONSTER)));
	hitDie = j.value("hitDie", 8);
	attacksPerRound = j.value("attacksPerRound", 1.0f);
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
	if (j.contains("inventoryData"))
	{
		inventoryData = InventoryData(50); // Default capacity
		InventoryOperations::load_inventory(inventoryData, j["inventoryData"]);
	}
	if (j.contains("shop"))
	{
		shop = ShopKeeper::create(j["shop"]);
	}

	// Load unified buff system
	if (j.contains("activeBuffs"))
	{
		activeBuffs.clear();
		for (const auto& buffJson : j["activeBuffs"])
		{
			Buff buff{};
			buff.type = static_cast<BuffType>(buffJson.at("type").get<int>());
			buff.value = buffJson.at("value").get<int>();
			buff.turnsRemaining = buffJson.at("turnsRemaining").get<int>();
			buff.isSetEffect = buffJson.at("isSetEffect").get<bool>();
			activeBuffs.push_back(buff);
		}
	}
}

void Creature::save(json& j)
{
	Actor::save(j); // Call base class save
	j["strength"] = baseStrength;
	j["dexterity"] = baseDexterity;
	j["constitution"] = baseConstitution;
	j["intelligence"] = baseIntelligence;
	j["wisdom"] = baseWisdom;
	j["charisma"] = baseCharisma;
	j["playerLevel"] = creatureLevel;
	j["gold"] = gold;
	j["gender"] = gender;
	j["weaponEquipped"] = weaponEquipped;
	j["creatureClass"] = static_cast<int>(creatureClass);
	j["hitDie"] = hitDie;
	j["attacksPerRound"] = attacksPerRound;
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
	InventoryOperations::save_inventory(inventoryData, inventoryJson);
	j["inventoryData"] = inventoryJson;
	if (shop)
	{
		json shopJson;
		shop->save(shopJson);
		j["shop"] = shopJson;
	}

	// Save unified buff system
	json buffsJson = json::array();
	for (const auto& buff : activeBuffs)
	{
		json buffJson;
		buffJson["type"] = static_cast<int>(buff.type);
		buffJson["value"] = buff.value;
		buffJson["turnsRemaining"] = buff.turnsRemaining;
		buffJson["isSetEffect"] = buff.isSetEffect;
		buffsJson.push_back(buffJson);
	}
	j["activeBuffs"] = buffsJson;
}

// the actor update
void Creature::update(GameContext& ctx)
{
	if (!invisibleTile.is_valid() && ctx.tileConfig)
	{
		invisibleTile = ctx.tileConfig->get("TILE_INVISIBLE");
	}
	ctx.buffSystem->restore_loaded_buff_states(*this); // Restore states after deserialization (idempotent)
	ctx.buffSystem->update_creature_buffs(*this); // Unified buff system

	assert(destructible && "Creature::update called with null destructible");
	assert(ai && "Creature::update called with null ai");

	destructible->update_armor_class(*this, ctx);
	destructible->update_constitution_bonus(*this, ctx);
	ai->update(*this, ctx);
}

void Creature::apply_confusion(int nbTurns)
{
	ai = std::make_unique<AiMonsterConfused>(nbTurns, std::move(ai));
}

void Creature::equip(Item& item, GameContext& ctx)
{
	bool isArmor = item.is_armor();
	bool isWeapon = item.is_weapon();
	bool isShield = item.is_shield();

	// First check if any equipment of the same type is already equipped
	std::vector<Item*> equippedItems;

	// Find all equipped items
	for (const auto& invItem : inventoryData.items)
	{
		if (invItem && invItem->has_state(ActorState::IS_EQUIPPED))
		{
			bool itemIsArmor = invItem->is_armor();
			bool itemIsWeapon = invItem->is_weapon();
			bool itemIsShield = invItem->is_shield();

			// Only consider same-type equipment for unequipping
			if ((isArmor && itemIsArmor) || (isWeapon && itemIsWeapon) || (isShield && itemIsShield))
			{
				equippedItems.push_back(invItem.get());
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
		weaponEquipped = item.get_name();
		std::string weaponDamage = WeaponDamageRegistry::get_damage_roll(item.item_key);
		ctx.messageSystem->log(std::format("Equipped {} - damage: {}", item.get_name(), weaponDamage));
	}

	// Log shield equipped
	if (isShield)
	{
		ctx.messageSystem->log(std::format("Equipped {}", item.get_name()));
	}

	// Log armor equipped
	if (isArmor)
	{
		ctx.messageSystem->log(std::format("Equipped {}", item.get_name()));
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
			ctx.messageSystem->log("Unequipped weapon - now unarmed");

			// Check for ranged weapon - use ItemClass system
			if (item.is_ranged_weapon())
			{
				// Remove the ranged state
				if (has_state(ActorState::IS_RANGED))
				{
					remove_state(ActorState::IS_RANGED);
					ctx.messageSystem->log("Removed IS_RANGED state after unequipping " + item.actorData.name);
				}
			}
		}

		// Double-check all inventory to see if we still should have IS_RANGED
		bool hasRangedWeapon = false;
		for (const auto& invItem : inventoryData.items)
		{
			if (invItem && invItem->has_state(ActorState::IS_EQUIPPED) && invItem->behavior)
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
			ctx.messageSystem->log("Force removed IS_RANGED state - no ranged weapons equipped");
		}
	}
}

void Creature::sync_ranged_state(GameContext& ctx)
{
	// Check if the MISSILE_WEAPON slot holds a ranged weapon
	Item* missileSlot = get_equipped_item(EquipmentSlot::MISSILE_WEAPON);
	bool hasRangedWeapon = missileSlot && missileSlot->is_ranged_weapon();

	// Make sure IS_RANGED state matches equipped weapons
	if (hasRangedWeapon && !has_state(ActorState::IS_RANGED))
	{
		add_state(ActorState::IS_RANGED);
		ctx.messageSystem->log("Added missing IS_RANGED state - ranged weapon equipped");
	}
	else if (!hasRangedWeapon && has_state(ActorState::IS_RANGED))
	{
		remove_state(ActorState::IS_RANGED);
		ctx.messageSystem->log("Removed incorrect IS_RANGED state - no ranged weapons equipped");
	}
}

void Creature::pick(GameContext& ctx)
{
	// Check if inventory is already full before attempting to pick
	if (InventoryOperations::is_inventory_full(inventoryData))
	{
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "Your inventory is full! You can't carry any more items.", true);
		return;
	}

	// Find item at player's position using proper inventory interface
	Item* itemAtPosition = nullptr;
	size_t itemIndex = 0;

	if (ctx.inventoryData->items.empty())
	{
		return; // No floor items
	}

	// Search for item at current position
	for (size_t i = 0; i < ctx.inventoryData->items.size(); ++i)
	{
		if (auto& item = ctx.inventoryData->items[i])
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
	if (itemAtPosition->itemClass == ItemClass::GOLD_COIN)
	{
		// Use the pickable's use() method which handles gold pickup properly
		if (itemAtPosition->behavior && use_item(*itemAtPosition->behavior, *itemAtPosition, *this, ctx))
		{
			// Gold was picked up successfully via polymorphic call
			// Remove gold from floor inventory
			auto removeResult = InventoryOperations::remove_item_at(*ctx.inventoryData, itemIndex);
			if (!removeResult.has_value())
			{
				ctx.messageSystem->log("WARNING: Failed to remove gold item from floor inventory");
			}
		}
	}
	else
	{
		// Normal item handling - store name before moving
		const std::string itemName = itemAtPosition->actorData.name;

		// Remove from floor first
		auto removeResult = InventoryOperations::remove_item_at(*ctx.inventoryData, itemIndex);
		if (removeResult.has_value())
		{
			// Add to player inventory
			auto addResult = InventoryOperations::add_item(inventoryData, std::move(*removeResult));
			if (addResult.has_value())
			{
				ctx.messageSystem->message(WHITE_BLACK_PAIR, "You picked up the " + itemName + ".", true);
			}
			else
			{
				ctx.messageSystem->log("WARNING: Failed to add item to player inventory");
				// Item is lost - this should not happen since we checked is_full() earlier
			}
		}
		else
		{
			ctx.messageSystem->log("WARNING: Failed to remove item from floor inventory");
		}
	}
}

void Creature::drop(Item& item, GameContext& ctx)
{
	// Check if the item is actually in the inventory first
	auto it = std::find_if(inventoryData.items.begin(), inventoryData.items.end(), [&item](const auto& invItem)
		{ return invItem.get() == &item; });

	if (it != inventoryData.items.end())
	{
		// Set the item's position to the player's position
		(*it)->position = position;

		// If the item is equipped, unequip it first
		if ((*it)->has_state(ActorState::IS_EQUIPPED))
		{
			unequip(*(*it), ctx);
		}

		// Add to game floor inventory
		auto addResult = InventoryOperations::add_item(*ctx.inventoryData, std::move(*it));
		if (addResult.has_value())
		{
			// Clean up null pointer that remains after moving
			InventoryOperations::optimize_inventory_storage(inventoryData);
			ctx.messageSystem->message(WHITE_BLACK_PAIR, "You dropped the item.", true);
		}
	}
}

TileRef Creature::get_display_tile() const noexcept
{
	if (is_invisible())
	{
		return invisibleTile; // Instant access, no string lookup
	}
	return Actor::get_display_tile();
}

int Creature::get_display_color() const noexcept
{
	if (is_invisible())
	{
		return CYAN_BLACK_PAIR;
	}
	return Actor::get_display_color();
}

//==Unified Buff System - Modifier Stack Pattern==
// Note: Buff lifecycle managed by BuffSystem, not Creature

// Helper to sum all buff values of a specific type using ranges
// AD&D 2e: Calculate effective stat value combining base, SET, and ADD effects
// LOGIC: effective = MAX(base, highest_SET) + SUM(all_ADDs)

int Creature::calculate_effective_stat(int base_value, BuffType type) const noexcept
{
	auto matchesType = [type](const Buff& b)
	{
		return b.type == type;
	};
	auto matchingBuffs = activeBuffs | std::views::filter(matchesType);

	int highestSet = 0; // Highest SET effect (Potion of Giant Strength → 18)
	int sumOfAdds = 0; // Sum of ADD effects (Strength spell +1, Gauntlets +2)

	for (const auto& buff : matchingBuffs)
	{
		if (buff.isSetEffect)
		{
			highestSet = std::max(highestSet, buff.value);
		}
		else
		{
			sumOfAdds += buff.value;
		}
	}

	// AD&D 2e: SET effects replace base (if higher), ADD effects always stack
	// Example: base=14, SET=18, ADD=+2 → MAX(14,18) + 2 = 20
	int effectiveBase = (highestSet > 0) ? std::max(base_value, highestSet) : base_value;
	return effectiveBase + sumOfAdds;
}