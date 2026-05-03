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
#include "../Systems/AnimationSystem.h"
#include "../Persistent/Persistent.h"
#include "../Systems/BuffSystem.h"
#include "../Systems/BuffType.h"
#include "../Systems/MessageSystem.h"
#include "../Systems/ShopKeeper.h"
#include "../Systems/TileConfig.h"
#include "Actor.h"
#include "Attacker.h"
#include "MonsterAttacker.h"
#include "EquipmentSlot.h"
#include "InventoryData.h"
#include "Item.h"
#include "Creature.h"

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
	damageResistance = j.value("dr", 0);
	thaco = j.value("thaco", 20);
	if (j.contains("attacker"))
	{
		attacker = std::make_unique<MonsterAttacker>(*this, DamageInfo{});
		attacker->load(j["attacker"]);
	}
	// Load health pool data
	if (j.contains("healthPool"))
	{
		const auto& healthJson = j["healthPool"];
		if (healthPool && healthJson.contains("hpMax"))
		{
			healthPool->set_max_hp(healthJson.at("hpMax").get<int>());
			healthPool->set_hp(healthJson.at("hp").get<int>());
			healthPool->set_hp_base(healthJson.at("hpBase").get<int>());
			healthPool->set_temp_hp(healthJson.at("tempHp").get<int>());
		}
	}
	// Load constitution tracker state
	if (j.contains("constitutionTracker"))
	{
		const auto& constJson = j["constitutionTracker"];
		if (constJson.contains("lastConstitution"))
		{
			constitutionTracker->set_last_constitution(constJson.at("lastConstitution").get<int>());
		}
	}
	// Load experience reward
	if (j.contains("experienceReward"))
	{
		experienceReward = std::make_unique<ExperienceReward>(0);
		experienceReward->load(j["experienceReward"]);
	}
	// Load armor class
	if (j.contains("armorClass"))
	{
		const auto& acJson = j["armorClass"];
		armorClass = std::make_unique<ArmorClass>(10);
		armorClass->set_armor_class(acJson.at("armorClass").get<int>());
		armorClass->set_base_armor_class(acJson.at("baseArmorClass").get<int>());
	}
	if (j.contains("ai"))
	{
		ai = Ai::create(j["ai"]);
	}
	if (j.contains("inventoryData"))
	{
		inventoryData = CreatureInventory(50); // Default capacity
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
	j["dr"] = damageResistance;
	j["thaco"] = thaco;
	if (attacker)
	{
		json attackerJson;
		attacker->save(attackerJson);
		// Only write the key if the strategy produced content.
		// PlayerAttacker::save() is a no-op — no damageInfo to persist.
		if (!attackerJson.empty())
		{
			j["attacker"] = attackerJson;
		}
	}
	// Save health pool data
	if (healthPool)
	{
		json healthJson;
		healthJson["hpMax"] = healthPool->get_max_hp();
		healthJson["hp"] = healthPool->get_hp();
		healthJson["hpBase"] = healthPool->get_hp_base();
		healthJson["tempHp"] = healthPool->get_temp_hp();
		j["healthPool"] = healthJson;
	}
	// Save constitution tracker state
	json constJson;
	constJson["lastConstitution"] = get_last_constitution();
	j["constitutionTracker"] = constJson;
	// Save experience reward
	if (experienceReward)
	{
		json expJson;
		experienceReward->save(expJson);
		j["experienceReward"] = expJson;
	}
	// Save armor class
	if (armorClass)
	{
		json acJson;
		acJson["armorClass"] = armorClass->get_armor_class();
		acJson["baseArmorClass"] = armorClass->get_base_armor_class();
		j["armorClass"] = acJson;
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

void Creature::update_creature_state(GameContext& ctx)
{
	if (!invisibleTile.is_valid() && ctx.tileConfig)
	{
		invisibleTile = ctx.tileConfig->get("TILE_INVISIBLE");
	}

	ctx.buffSystem->restore_loaded_buff_states(*this);
	ctx.buffSystem->update_creature_buffs(*this);

	update_armor_class(ctx);
	update_constitution_bonus(ctx);
}

void Creature::update_constitution_bonus(GameContext& ctx)
{
	const int oldCon = get_last_constitution();
	const auto result = constitutionTracker->apply_constitution_changes(*this, ctx);

	if (result.hpDifference == 0)
	{
		return;
	}

	set_max_hp(get_max_hp() + result.hpDifference);
	set_hp(get_hp() + result.hpDifference);

	// Log only for player
	if (this == ctx.player)
	{
		if (result.hpDifference > 0)
		{
			ctx.messageSystem->message(GREEN_BLACK_PAIR,
				std::format("Constitution increased from {} to {}! You gain {} hit points.", oldCon, get_constitution(), result.hpDifference),
				true);
		}
		else
		{
			ctx.messageSystem->message(RED_BLACK_PAIR,
				std::format("Constitution decreased from {} to {}! You lose {} hit points.", oldCon, get_constitution(), -result.hpDifference),
				true);
		}
	}

	if (get_hp() <= 0)
	{
		set_hp(0);
		if (this == ctx.player)
		{
			ctx.messageSystem->message(RED_BLACK_PAIR, "Your life force has been drained beyond recovery. You die!", true);
		}
		else
		{
			ctx.messageSystem->log(std::format("{} dies from stat drain.", get_name()));
		}
		die(ctx);
	}
}

int Creature::take_damage(int damage, GameContext& ctx, DamageType damageType)
{
	return healthPool->take_damage(*this, damage, ctx, damageType);
}

void Creature::take_damage_and_check_death(int damage, GameContext& ctx, DamageType damageType)
{
	take_damage(damage, ctx, damageType);
	if (is_dead())
	{
		die(ctx);
	}
}

// the actor update -- monsters only; Player overrides this
void Creature::update(GameContext& ctx)
{
	update_creature_state(ctx);
	assert(ai && "Creature::update called with null ai");
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
	assert(std::ranges::none_of(inventoryData.items, [](const auto& i) { return !i; }));
	for (const auto& invItem : inventoryData.items)
	{
		if (invItem->has_state(ActorState::IS_EQUIPPED))
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
		std::string weaponDamage = WeaponDamageRegistry::get_damage_roll(item.itemKey);
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
		assert(std::ranges::none_of(inventoryData.items, [](const auto& i) { return !i; }));
		for (const auto& invItem : inventoryData.items)
		{
			if (invItem->has_state(ActorState::IS_EQUIPPED) && invItem->behavior)
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


void Creature::drop(Item& item, GameContext& ctx)
{
	auto is_null = [](const auto& invItem) { return !invItem; };
	assert(std::ranges::none_of(inventoryData.items, is_null));

	auto matches_item = [&item](const auto& invItem) { return invItem.get() == &item; };
	auto matches = inventoryData.items | std::views::filter(matches_item);

	if (std::ranges::empty(matches))
	{
		return;
	}

	auto& foundPtr = matches.front();
	foundPtr->position = position;

	if (foundPtr->has_state(ActorState::IS_EQUIPPED))
	{
		unequip(*foundPtr, ctx);
	}

	auto addResult = InventoryOperations::add_item(*ctx.floorInventory, std::move(foundPtr));
	if (addResult.has_value())
	{
		InventoryOperations::optimize_inventory_storage(inventoryData);
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "You dropped the item.", true);
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

void Creature::die(GameContext& ctx)
{
	// Monster death: message, reward, animation, drop items, create corpse
	ctx.messageSystem->append_message_part(actorData.color, std::format("{}", actorData.name));
	ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, " is dead.\n");
	ctx.messageSystem->finalize_message();

	ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, "You get ");
	ctx.messageSystem->append_message_part(YELLOW_BLACK_PAIR, std::format("{}", get_xp()));
	ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, " experience points.\n");
	ctx.messageSystem->finalize_message();

	assert(ctx.player != nullptr && "Creature::die requires a live player in context");
	ctx.player->on_kill_reward(get_xp(), ctx);

	if (ctx.animSystem)
	{
		ctx.animSystem->spawn_death(position.x, position.y);
	}

	assert(std::ranges::none_of(inventoryData.items, [](const auto& i) { return !i; }));
	for (auto& item : inventoryData.items)
	{
		item->position = position;
		assert(InventoryOperations::add_item(*ctx.floorInventory, std::move(item)).has_value());
	}
	inventoryData.items.clear();

	// Create a corpse on the floor in place of the creature.
	auto corpse = std::make_unique<Item>(position, actorData);
	corpse->actorData.name = std::format("dead {}", get_name());
	corpse->actorData.tile = ctx.tileConfig->get("TILE_CORPSE");
	corpse->enhancement.weight = get_corpse_weight();
	corpse->behavior = CorpseFood{ 0 };
	assert(InventoryOperations::add_item(*ctx.floorInventory, std::move(corpse)).has_value());
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