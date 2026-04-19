// file: Player.cpp
#include <algorithm>
#include <cstdint>
#include <format>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

#include "../Actor/Actor.h"
#include "../Actor/PlayerAttacker.h"
#include "../Actor/Destructible.h"
#include "../Actor/EquipmentSlot.h"
#include "../Actor/InventoryOperations.h"
#include "../Actor/Pickable.h"
#include "../Ai/Ai.h"
#include "../Colors/Colors.h"
#include "../Combat/WeaponDamageRegistry.h"
#include "../Core/GameContext.h"
#include "../dnd_tables/CombatProgressionTables.h"
#include "../Factories/ItemCreator.h"
#include "../Items/ItemClassification.h"
#include "../Map/Map.h"
#include "../Objects/Web.h"
#include "../Persistent/Persistent.h"
#include "../Random/RandomDice.h"
#include "../Renderer/Renderer.h"
#include "../Systems/BuffSystem.h"
#include "../Systems/BuffType.h"
#include "../Systems/DisplayManager.h"
#include "../Systems/HungerSystem.h"
#include "../Systems/MessageSystem.h"
#include "../Systems/RenderingManager.h"
#include "../Menu/NotificationMenu.h"
#include "../Systems/SpellSystem.h"
#include "../Utils/Vector2D.h"
#include "Player.h"

// XP table helpers — pure functions, no state
namespace
{
template <std::size_t N>
constexpr int calculate_xp_for_level(int level, const std::array<int, N>& xpTable, int linearProgression) noexcept
{
	if (level <= 0)
		return xpTable[0];

	if (level < static_cast<int>(xpTable.size()))
		return xpTable[level];

	const int maxLevel = static_cast<int>(xpTable.size()) - 1;
	return xpTable[maxLevel] + (level - maxLevel) * linearProgression;
}

constexpr int calculate_fighter_xp(int level) noexcept
{
	constexpr std::array fighter_xp = {
		0, 2000, 4000, 8000, 16000, 32000, 64000, 125000, 250000, 500000, 750000
	};
	return calculate_xp_for_level(level, fighter_xp, 250000);
}

constexpr int calculate_rogue_xp(int level) noexcept
{
	constexpr std::array rogue_xp = {
		0, 1250, 2500, 5000, 10000, 20000, 40000, 70000, 110000, 160000, 220000
	};
	return calculate_xp_for_level(level, rogue_xp, 60000);
}

constexpr int calculate_cleric_xp(int level) noexcept
{
	constexpr std::array cleric_xp = {
		0, 1500, 3000, 6000, 13000, 27500, 55000, 110000, 225000, 450000, 675000
	};
	return calculate_xp_for_level(level, cleric_xp, 225000);
}

constexpr int calculate_wizard_xp(int level) noexcept
{
	constexpr std::array wizard_xp = {
		0, 2500, 5000, 10000, 20000, 40000, 60000, 90000, 135000, 250000, 375000
	};
	return calculate_xp_for_level(level, wizard_xp, 125000);
}
} // namespace (xp helpers)

// Equipment comparison predicates for DRY compliance
namespace
{
constexpr auto matches_slot = [](EquipmentSlot slot)
{
	return [slot](const EquippedItem& equipped)
	{
		return equipped.slot == slot;
	};
};

constexpr auto matches_unique_id = [](uint64_t unique_id)
{
	return [unique_id](const EquippedItem& equipped)
	{
		return equipped.item->uniqueId == unique_id;
	};
};
} // namespace

Player::Player(Vector2D position)
	: Creature(position, ActorData{ TileRef{}, "Player", WHITE_BLACK_PAIR })
{
	set_gold(100);
	controller = std::make_unique<PlayerController>(*this);
}

Player::Player(Vector2D position, const PlayerBlueprint& blueprint, GameContext& ctx)
	: Creature(position, ActorData{ TileRef{}, blueprint.name, WHITE_BLACK_PAIR })
{
	set_gender(blueprint.gender);
	playerClass = blueprint.playerClass;
	playerRace = blueprint.playerRace;

	auto applyClassData = [this](std::string_view name)
	{
		if (name == "Fighter")
		{
			playerClassState = PlayerClassState::FIGHTER;
			set_creature_class(CreatureClass::FIGHTER);
			set_hit_die(10);
		}
		else if (name == "Rogue")
		{
			playerClassState = PlayerClassState::ROGUE;
			set_creature_class(CreatureClass::ROGUE);
			set_hit_die(6);
		}
		else if (name == "Cleric")
		{
			playerClassState = PlayerClassState::CLERIC;
			set_creature_class(CreatureClass::CLERIC);
			set_hit_die(8);
		}
		else if (name == "Wizard")
		{
			playerClassState = PlayerClassState::WIZARD;
			set_creature_class(CreatureClass::WIZARD);
			set_hit_die(4);
		}
	};

	auto applyRaceData = [this](std::string_view name)
	{
		if (name == "Human")
		{
			playerRaceState = PlayerRaceState::HUMAN;
		}
		else if (name == "Dwarf")
		{
			playerRaceState = PlayerRaceState::DWARF;
		}
		else if (name == "Elf")
		{
			playerRaceState = PlayerRaceState::ELF;
		}
		else if (name == "Gnome")
		{
			playerRaceState = PlayerRaceState::GNOME;
		}
		else if (name == "Half-Elf")
		{
			playerRaceState = PlayerRaceState::HALFELF;
		}
		else if (name == "Halfling")
		{
			playerRaceState = PlayerRaceState::HALFLING;
		}
	};

	applyClassData(blueprint.playerClass);
	applyRaceData(blueprint.playerRace);

	set_gold(100);
	controller = std::make_unique<PlayerController>(*this);
	roll_new_character(ctx);

	assert(attacker && "Player requires Attacker");
	assert(destructible && "Player requires Destructible");
}

void Player::roll_new_character(GameContext& ctx)
{
	// Rolling for stats
	auto roll3d6 = [&ctx]()
	{
		return ctx.dice->d6() + ctx.dice->d6() + ctx.dice->d6();
	};

	set_strength(roll3d6());
	set_dexterity(roll3d6());
	set_constitution(roll3d6());
	set_intelligence(roll3d6());
	set_wisdom(roll3d6());
	set_charisma(roll3d6());

	const int playerHp = 20 + ctx.dice->d10();
	const int playerDr = 1;
	const int playerXp = 0;
	const int playerAC = 10;

	attacker = std::make_unique<PlayerAttacker>(*this);
	destructible = std::make_unique<PlayerDestructible>(
		playerHp, playerDr, "your corpse", playerXp, 0, playerAC);
}

void Player::on_new_game_start(GameContext& ctx)
{
	racial_ability_adjustments(ctx);
	equip_class_starting_gear(ctx);
}

void Player::recalculate_combat_stats()
{
	calculate_thaco();
}

void Player::on_kill_reward(int xp, GameContext& ctx)
{
	destructible->set_xp(destructible->get_xp() + xp);
	++killCount;
	levelup_update(ctx);
}

void Player::equip_class_starting_gear(GameContext& ctx)
{
	switch (playerClassState)
	{

	case PlayerClassState::FIGHTER:
	{
		int startingGold = (ctx.dice->d4() + ctx.dice->d4() + ctx.dice->d4() + ctx.dice->d4() + ctx.dice->d4()) * 10;
		set_gold(startingGold);
		equip_item(ItemCreator::create("plate_mail", position, *ctx.contentRegistry), EquipmentSlot::BODY, ctx);
		equip_item(ItemCreator::create("long_sword", position, *ctx.contentRegistry), EquipmentSlot::RIGHT_HAND, ctx);
		equip_item(ItemCreator::create("medium_shield", position, *ctx.contentRegistry), EquipmentSlot::LEFT_HAND, ctx);
		equip_item(ItemCreator::create("long_bow", position, *ctx.contentRegistry), EquipmentSlot::MISSILE_WEAPON, ctx);
		InventoryOperations::add_item(*ctx.inventoryData, ItemCreator::create("scroll_fireball", position, *ctx.contentRegistry));
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "Fighter equipped with plate mail, long sword, shield, long bow, fireball scroll. [DEBUG]", true);
		break;
	}

	case PlayerClassState::ROGUE:
	{
		int startingGold = (ctx.dice->d6() + ctx.dice->d6()) * 10;
		set_gold(startingGold);
		equip_item(ItemCreator::create("leather_armor", position, *ctx.contentRegistry), EquipmentSlot::BODY, ctx);
		equip_item(ItemCreator::create("dagger", position, *ctx.contentRegistry), EquipmentSlot::RIGHT_HAND, ctx);
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "Rogue equipped with leather armor and dagger.", true);
		break;
	}

	case PlayerClassState::CLERIC:
	{
		int startingGold = (ctx.dice->d6() + ctx.dice->d6() + ctx.dice->d6()) * 10;
		set_gold(startingGold);
		equip_item(ItemCreator::create("chain_mail", position, *ctx.contentRegistry), EquipmentSlot::BODY, ctx);
		equip_item(ItemCreator::create("mace", position, *ctx.contentRegistry), EquipmentSlot::RIGHT_HAND, ctx);
		equip_item(ItemCreator::create("medium_shield", position, *ctx.contentRegistry), EquipmentSlot::LEFT_HAND, ctx);
		InventoryOperations::add_item(*ctx.inventoryData, ItemCreator::create("health_potion", position, *ctx.contentRegistry));
		InventoryOperations::add_item(*ctx.inventoryData, ItemCreator::create("scroll_hold_person", position, *ctx.contentRegistry));
		SpellSystem::show_memorization_menu(*this, ctx);
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "Cleric equipped with chain mail, mace, and shield. Spells memorized.", true);
		break;
	}

	case PlayerClassState::WIZARD:
	{
		int startingGold = (ctx.dice->d4() + ctx.dice->d4()) * 10;
		set_gold(startingGold);
		equip_item(ItemCreator::create("staff", position, *ctx.contentRegistry), EquipmentSlot::RIGHT_HAND, ctx);
		InventoryOperations::add_item(*ctx.inventoryData, ItemCreator::create("scroll_fireball", position, *ctx.contentRegistry));
		InventoryOperations::add_item(*ctx.inventoryData, ItemCreator::create("scroll_lightning", position, *ctx.contentRegistry));
		InventoryOperations::add_item(*ctx.inventoryData, ItemCreator::create("scroll_sleep", position, *ctx.contentRegistry));
		SpellSystem::show_memorization_menu(*this, ctx);
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "Wizard equipped with staff. Attack scrolls and spells ready.", true);
		break;
	}

	default:
	{
		break;
	}

	}
}
void Player::racial_ability_adjustments(GameContext& ctx)
{
	// Apply stat changes and collect the display lines in a single pass.
	// Human and Half-elf have no stat modifications (AD&D 2e PHB), so they
	// produce no entries and no popup is shown.
	std::vector<std::string> bonusLines;

	switch (playerRaceState)
	{

	case Player::PlayerRaceState::HUMAN:
	case Player::PlayerRaceState::HALFELF:
	{
		break;
	}

	case Player::PlayerRaceState::DWARF:
	{
		adjust_constitution(1);
		adjust_charisma(-1);
		bonusLines.push_back("+1 Constitution");
		bonusLines.push_back("-1 Charisma");
		break;
	}

	case Player::PlayerRaceState::ELF:
	{
		adjust_dexterity(1);
		adjust_constitution(-1);
		bonusLines.push_back("+1 Dexterity");
		bonusLines.push_back("-1 Constitution");
		break;
	}

	case Player::PlayerRaceState::GNOME:
	{
		adjust_intelligence(1);
		adjust_wisdom(-1);
		bonusLines.push_back("+1 Intelligence");
		bonusLines.push_back("-1 Wisdom");
		break;
	}

	case Player::PlayerRaceState::HALFLING:
	{
		adjust_dexterity(1);
		adjust_strength(-1);
		bonusLines.push_back("+1 Dexterity");
		bonusLines.push_back("-1 Strength");
		break;
	}

	default:
	{
		break;
	}

	}

	if (bonusLines.empty())
	{
		return;
	}

	ctx.menus->push_back(std::make_unique<NotificationMenu>(
		std::format("Racial Traits: {}", playerRace),
		std::move(bonusLines),
		ctx));
}

void Player::calculate_thaco()
{
	// Static instance for efficiency - no need to recreate each time
	static constexpr CombatProgressionTables combatTables;

	switch (playerClassState)
	{

	case PlayerClassState::FIGHTER:
	{
		destructible->set_thaco(combatTables.get_fighter(get_creature_level()));
		break;
	}

	case PlayerClassState::ROGUE:
	{
		destructible->set_thaco(combatTables.get_rogue(get_creature_level()));
		break;
	}

	case PlayerClassState::CLERIC:
	{
		destructible->set_thaco(combatTables.get_cleric(get_creature_level()));
		break;
	}

	case PlayerClassState::WIZARD:
	{
		destructible->set_thaco(combatTables.get_wizard(get_creature_level()));
		break;
	}

	case PlayerClassState::NONE:
	{
		break;
	}

	default:
	{
		break;
	}

	}
}

void Player::consume_food(int nutrition, GameContext& ctx)
{
	// Decrease hunger by nutrition value
	ctx.hungerSystem->decrease_hunger(ctx, nutrition);
}

void Player::render(const GameContext& ctx) const noexcept
{
	// First, render the player normally
	Creature::render(ctx);
}

bool Player::rest(GameContext& ctx)
{
	// Check if player is already at full health
	if (destructible->get_hp() >= destructible->get_max_hp())
	{
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "You're already at full health.", true);
		return false;
	}

	// Check if enemies are nearby (within a radius of 5 tiles)
	// Exclude shopkeepers and other neutral NPCs
	for (const auto& creature : *ctx.creatures)
	{
		if (creature && !creature->destructible->is_dead())
		{
			// Skip the player themselves
			if (creature.get() == this)
			{
				continue;
			}

			// Skip non-hostile creatures (shopkeepers, etc.)
			if (creature->ai && !creature->ai->is_hostile())
			{
				continue;
			}

			// Calculate distance to creature
			int distance = get_tile_distance(creature->position);

			// If hostile enemy is within 5 tiles, can't rest
			if (distance <= 5)
			{
				ctx.messageSystem->message(WHITE_BLACK_PAIR, "You can't rest with enemies nearby!", true);
				return false;
			}
		}
	}

	// Check if player has enough food (hunger isn't too high)
	if (ctx.hungerSystem->get_hunger_state() == HungerState::STARVING ||
		ctx.hungerSystem->get_hunger_state() == HungerState::DYING)
	{
		ctx.messageSystem->message(WHITE_RED_PAIR, "You're too hungry to rest!", true);
		return false;
	}

	// Show resting animation
	animate_resting(ctx);

	// Rest - heal 20% of max HP
	int healAmount = std::max(1, destructible->get_max_hp() / 5);
	int amountHealed = destructible->heal(healAmount);

	// Capture the hunger state before and after
	HungerState beforeState = ctx.hungerSystem->get_hunger_state();

	// Consume food (increase hunger)
	const int hungerCost = 50;
	ctx.hungerSystem->increase_hunger(ctx, hungerCost);

	HungerState afterState = ctx.hungerSystem->get_hunger_state();

	// Display message with more detail
	ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, "Resting recovers ");
	ctx.messageSystem->append_message_part(WHITE_GREEN_PAIR, std::to_string(amountHealed));
	ctx.messageSystem->append_message_part(WHITE_GREEN_PAIR, " health");

	if (beforeState != afterState)
	{
		// If hunger state changed, mention it
		ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, ", but you've become ");
		ctx.messageSystem->append_message_part(ctx.hungerSystem->get_hunger_color(), ctx.hungerSystem->get_hunger_state_string().c_str());
		ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, ".");
	}
	else
	{
		ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, ", consuming your food.");
	}

	ctx.messageSystem->finalize_message();

	// Memorize spells for casters
	if (playerClassState == PlayerClassState::CLERIC || playerClassState == PlayerClassState::WIZARD)
	{
		SpellSystem::show_memorization_menu(*this, ctx);
	}

	// Resting takes time
	ctx.gameState->set_game_status(GameStatus::NEW_TURN);
	return true;
}

void Player::animate_resting(GameContext& ctx)
{
	// TODO: Implement resting animation via Renderer
	// Should display 'z', 'z', 'Z', 'Z', 'Z' symbols above player
	// with WHITE_GREEN_PAIR color and 80ms delay between frames
	ctx.renderingManager->render(ctx);
}

bool Player::attempt_hide(GameContext& ctx)
{
	// Only rogues can hide
	if (playerClassState != PlayerClassState::ROGUE)
	{
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "Only rogues can hide in shadows.", true);
		return false;
	}

	// Already invisible
	if (is_invisible())
	{
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "You are already hidden.", true);
		return false;
	}

	// Check if enemies can see player
	bool observed = false;
	for (const auto& creature : *ctx.creatures)
	{
		if (creature && creature->destructible && !creature->destructible->is_dead())
		{
			if (ctx.map->is_in_fov(creature->position))
			{
				observed = true;
				break;
			}
		}
	}

	if (observed)
	{
		ctx.messageSystem->message(RED_BLACK_PAIR, "You cannot hide while being observed!", true);
		return false;
	}

	// Success - hide duration based on level
	int hideDuration = 10 + get_creature_level() * 2;
	ctx.buffSystem->add_buff(*this, BuffType::INVISIBILITY, 0, hideDuration, false);
	ctx.messageSystem->message(CYAN_BLACK_PAIR, std::format("You melt into the shadows... (Hidden for {} turns)", hideDuration), true);
	return true;
}

void Player::apply_web_effect(int duration, int strength, Web* web, GameContext& ctx)
{
	webStuckTurns = duration;
	webStrength = strength;
	trappingWeb = web;

	ctx.messageSystem->message(WHITE_BLACK_PAIR, "You're caught in a sticky web for " + std::to_string(duration) + " turns!", true);
}

bool Player::try_break_web(GameContext& ctx)
{
	// Calculate chance to break free based on strength vs web strength
	int breakChance = 20 + (get_strength() * 5) - (webStrength * 10);

	// Ensure some minimum chance
	breakChance = std::max(10, breakChance);

	// Roll to break free
	if (ctx.dice->d100() <= breakChance)
	{
		// Success!
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "You break free from the web!", true);

		// Destroy the web that trapped the player
		if (trappingWeb)
		{
			trappingWeb->destroy(ctx);
			trappingWeb = nullptr;
		}

		webStuckTurns = 0;
		webStrength = 0;
		return true;
	}

	// Still stuck
	webStuckTurns--;
	if (webStuckTurns <= 0)
	{
		// Time expired, free anyway
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "You finally break free from the web!", true);

		// Destroy the web that trapped the player
		if (trappingWeb)
		{
			trappingWeb->destroy(ctx);
			trappingWeb = nullptr;
		}

		webStuckTurns = 0;
		webStrength = 0;
		return true;
	}

	ctx.messageSystem->message(WHITE_BLACK_PAIR, "You're still stuck in the web. Turns remaining: " + std::to_string(webStuckTurns), true);
	return false;
}

// Clean Weapon Equipment System using Unique IDs
bool Player::toggle_weapon(uint64_t item_unique_id, EquipmentSlot preferred_slot, GameContext& ctx)
{
	// Check if weapon is already equipped
	if (is_item_equipped(item_unique_id))
	{
		// Find which slot and unequip
		for (auto slot : { EquipmentSlot::RIGHT_HAND, EquipmentSlot::LEFT_HAND, EquipmentSlot::MISSILE_WEAPON })
		{
			Item* equipped = get_equipped_item(slot);
			if (equipped && equipped->uniqueId == item_unique_id)
			{
				return unequip_item(slot, ctx);
			}
		}
	}
	else
	{
		// Find item in inventory and equip it
		Item* itemToEquip = InventoryOperations::find_item_by_id(inventoryData, item_unique_id);
		if (itemToEquip)
		{
			// Remove item from inventory
			auto result = InventoryOperations::remove_item_by_id(inventoryData, item_unique_id);
			if (result.has_value())
			{
				auto itemToEquip = std::move(*result);

				// Determine appropriate slot based on item classification
				EquipmentSlot target_slot;

				if (ItemClassificationUtils::is_ranged_weapon(itemToEquip->itemClass))
				{
					target_slot = EquipmentSlot::MISSILE_WEAPON;
				}
				else
				{
					target_slot = preferred_slot;
				}

				return equip_item(std::move(itemToEquip), target_slot, ctx);
			}
		}
	}
	return false;
}

bool Player::toggle_shield(uint64_t item_unique_id, GameContext& ctx)
{
	// Shields always go in LEFT_HAND slot
	if (is_item_equipped(item_unique_id))
	{
		return unequip_item(EquipmentSlot::LEFT_HAND, ctx);
	}
	else
	{
		auto result = InventoryOperations::remove_item_by_id(inventoryData, item_unique_id);
		if (result.has_value())
		{
			return equip_item(std::move(*result), EquipmentSlot::LEFT_HAND, ctx);
		}
		return false;
	}
}

bool Player::toggle_equipment(uint64_t item_unique_id, EquipmentSlot slot, GameContext& ctx)
{
	// Check if item is already equipped
	if (is_item_equipped(item_unique_id))
	{
		// Unequip the item
		return unequip_item(slot, ctx);
	}
	else
	{
		auto result = InventoryOperations::remove_item_by_id(inventoryData, item_unique_id);
		if (result.has_value())
		{
			return equip_item(std::move(*result), slot, ctx);
		}
		return false;
	}
}

bool Player::equip_item(std::unique_ptr<Item> item, EquipmentSlot slot, GameContext& ctx)
{
	if (!item)
	{
		if (ctx.messageSystem->is_debug_mode())
			ctx.messageSystem->log("DEBUG: equip_item failed - null item");
		return false;
	}

	if (!can_equip(*item, slot))
	{
		if (ctx.messageSystem->is_debug_mode())
		{
			ctx.messageSystem->log("DEBUG: can_equip failed for " + item->actorData.name);
			ctx.messageSystem->log("DEBUG: Item class: " + std::to_string(static_cast<int>(item->itemClass)));
			ctx.messageSystem->log("DEBUG: Is armor: " + std::string(item->is_armor() ? "true" : "false"));
			ctx.messageSystem->log("DEBUG: Slot: " + std::to_string(static_cast<int>(slot)));
			ctx.messageSystem->log("DEBUG: equip_item failed - can_equip returned false for " + item->actorData.name + " in slot " + std::to_string(static_cast<int>(slot)));
		}
		// Return item to inventory since we can't equip it
		InventoryOperations::add_item(inventoryData, std::move(item));
		return false;
	}

	// Unequip existing item in the slot first
	unequip_item(slot, ctx);

	// Special handling for two-handed weapons - use ItemClass system
	if (slot == EquipmentSlot::RIGHT_HAND)
	{
		if (item->is_two_handed_weapon())
		{
			// Two-handed weapon - also unequip left hand
			unequip_item(EquipmentSlot::LEFT_HAND, ctx);
			ctx.messageSystem->message(WHITE_BLACK_PAIR, "You grip the " + item->actorData.name + " with both hands.", true);
		}
	}

	// Add equipped item
	equippedItems.emplace_back(std::move(item), slot);

	// Mark item as equipped
	equippedItems.back().item->add_state(ActorState::IS_EQUIPPED);

	// Log weapon equip
	if (slot == EquipmentSlot::RIGHT_HAND && equippedItems.back().item->is_weapon())
	{
		std::string weaponDamage = WeaponDamageRegistry::get_damage_roll(equippedItems.back().item->item_key);
		ctx.messageSystem->log("Equipped " + equippedItems.back().item->actorData.name + " - damage: " + weaponDamage);
	}

	// Update armor class if armor or shield was equipped
	if (slot == EquipmentSlot::BODY || slot == EquipmentSlot::LEFT_HAND)
	{
		destructible->update_armor_class(*this, ctx);
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "Your armor class is now " + std::to_string(destructible->get_armor_class()) + ".", true);
	}

	// Set ranged state when a ranged weapon is equipped
	if (slot == EquipmentSlot::MISSILE_WEAPON)
		add_state(ActorState::IS_RANGED);

	return true;
}

// Equipment system implementation
bool Player::can_equip(const Item& item, EquipmentSlot slot) const noexcept
{
	// Basic slot validation
	if (slot == EquipmentSlot::NONE)
	{
		return false;
	}

	// Check if item has behavior component (weapons/armor)
	if (!item.behavior)
	{
		return false;
	}

	// Slot-specific validation
	switch (slot)
	{

	case EquipmentSlot::RIGHT_HAND:
	case EquipmentSlot::LEFT_HAND:
	{
		// Hand slots can hold weapons or shields - use proper item type system
		if (!item.is_weapon() && !item.is_shield())
		{
			return false; // Not a weapon or shield
		}

		// Shields can only go in left hand
		if (item.is_shield() && slot != EquipmentSlot::LEFT_HAND)
		{
			return false;
		}

		// Two-handed weapons can only go in right hand
		if (item.is_two_handed_weapon() && slot != EquipmentSlot::RIGHT_HAND)
		{
			return false;
		}

		// Check if trying to equip something in left hand when two-handed weapon is equipped
		if (slot == EquipmentSlot::LEFT_HAND)
		{
			auto* rightHandItem = get_equipped_item(EquipmentSlot::RIGHT_HAND);
			if (rightHandItem && rightHandItem->is_two_handed_weapon())
			{
				return false; // Can't equip anything in left hand when two-handed weapon equipped
			}
		}

		break;
	}

	case EquipmentSlot::BODY:
	{
		// Body slot can only hold armor - use proper item type system
		if (!item.is_armor())
		{
			return false;
		}
		break;
	}

	case EquipmentSlot::MISSILE_WEAPON:
	{
		// Missile weapon slot can only hold ranged weapons - use ItemClass system
		if (!item.is_ranged_weapon())
		{
			return false;
		}
		break;
	}

	case EquipmentSlot::HEAD:
	{
		if (!item.is_helmet())
		{
			return false;
		}
		break;
	}

	case EquipmentSlot::NECK:
	{
		if (!item.is_amulet())
		{
			return false;
		}
		break;
	}

	case EquipmentSlot::RIGHT_RING:
	case EquipmentSlot::LEFT_RING:
	{
		if (!item.is_ring())
		{
			return false;
		}
		break;
	}

	case EquipmentSlot::GAUNTLETS:
	{
		if (!item.is_gauntlets())
		{
			return false;
		}
		break;
	}

	case EquipmentSlot::GIRDLE:
	{
		if (!item.is_girdle())
		{
			return false;
		}
		break;
	}

	case EquipmentSlot::TOOL:
	{
		if (!item.is_tool())
		{
			return false;
		}
		break;
	}

	default:
	{
		// Other slots (CLOAK, BRACERS, BOOTS, MISSILES) - no items defined yet
		break;
	}

	}

	return true;
}

bool Player::unequip_item(EquipmentSlot slot, GameContext& ctx)
{
	// C++20 ranges - modern approach for finding elements
	auto it = std::ranges::find_if(equippedItems, matches_slot(slot));

	if (it != equippedItems.end())
	{
		remove_stat_bonuses_from_equipment(*it->item);

		// Remove equipped state
		it->item->remove_state(ActorState::IS_EQUIPPED);

		// Reset ranged state when the missile weapon slot is vacated
		if (slot == EquipmentSlot::MISSILE_WEAPON)
			remove_state(ActorState::IS_RANGED);

		// Return item to inventory
		InventoryOperations::add_item(inventoryData, std::move(it->item));

		// Remove from equipped items
		equippedItems.erase(it);

		// Update armor class if armor or shield was unequipped
		if (slot == EquipmentSlot::BODY || slot == EquipmentSlot::LEFT_HAND)
		{
			destructible->update_armor_class(*this, ctx);
			ctx.messageSystem->message(WHITE_BLACK_PAIR, "Your armor class is now " + std::to_string(destructible->get_armor_class()) + ".", true);
		}

		return true;
	}

	return false;
}

Item* Player::get_equipped_item(EquipmentSlot slot) const noexcept
{
	// C++20 ranges - modern approach for finding elements
	auto it = std::ranges::find_if(equippedItems, matches_slot(slot));

	return (it != equippedItems.end()) ? it->item.get() : nullptr;
}

bool Player::is_slot_occupied(EquipmentSlot slot) const noexcept
{
	return get_equipped_item(slot) != nullptr;
}

bool Player::is_dual_wielding() const noexcept
{
	// Check if both hands have weapons equipped
	auto rightHand = get_equipped_item(EquipmentSlot::RIGHT_HAND);
	auto leftHand = get_equipped_item(EquipmentSlot::LEFT_HAND);

	if (!rightHand || !leftHand)
	{
		return false;
	}

	// Check if left hand item is a weapon (not a shield) - use ItemClass system
	if (leftHand->is_shield())
	{
		return false; // Shield, not dual wielding
	}

	// Check if left hand item is a weapon
	if (leftHand->is_weapon())
	{
		return true; // Both hands have weapons
	}

	return false;
}

std::string Player::get_equipped_weapon_damage_roll() const noexcept
{
	auto rightHandWeapon = get_equipped_item(EquipmentSlot::RIGHT_HAND);
	if (!rightHandWeapon)
	{
		return WeaponDamageRegistry::get_unarmed_damage();
	}

	// Use pure ItemClass system
	if (rightHandWeapon->is_weapon())
	{
		return WeaponDamageRegistry::get_damage_roll(rightHandWeapon->item_key);
	}

	return WeaponDamageRegistry::get_unarmed_damage();
}

Player::DualWieldInfo Player::get_dual_wield_info() const noexcept
{
	DualWieldInfo info;

	// Check if dual wielding
	if (!is_dual_wielding())
	{
		return info; // Not dual wielding
	}

	info.isDualWielding = true;

	// AD&D 2e Two-Weapon Fighting penalties
	// Base penalties: -2 main hand, -4 off-hand
	info.mainHandPenalty = -2;
	info.offHandPenalty = -4;

	// Fighters have Two-Weapon Fighting proficiency: penalties reduce to 0/-2
	if (playerClassState == PlayerClassState::FIGHTER)
	{
		info.mainHandPenalty = 0;
		info.offHandPenalty = -2;
	}

	// Get off-hand weapon damage roll - use pure ItemClass system
	auto leftHandWeapon = get_equipped_item(EquipmentSlot::LEFT_HAND);
	if (leftHandWeapon)
	{
		if (leftHandWeapon->is_weapon())
		{
			info.offHandDamageRoll = WeaponDamageRegistry::get_damage_roll(leftHandWeapon->item_key);
		}
	}

	return info;
}

// Clean Equipment System using Unique IDs
bool Player::toggle_armor(uint64_t item_unique_id, GameContext& ctx)
{
	// Check if item is already equipped
	if (is_item_equipped(item_unique_id))
	{
		// Unequip the armor
		return unequip_item(EquipmentSlot::BODY, ctx);
	}
	else
	{
		auto result = InventoryOperations::remove_item_by_id(inventoryData, item_unique_id);
		if (result.has_value())
		{
			return equip_item(std::move(*result), EquipmentSlot::BODY, ctx);
		}
		return false;
	}
}

bool Player::is_item_equipped(uint64_t item_unique_id) const noexcept
{
	return std::ranges::any_of(equippedItems, matches_unique_id(item_unique_id));
}

int Player::get_constitution_hp_multiplier() const noexcept
{
	const int level = get_creature_level();

	// AD&D 2e: Constitution HP bonus per level caps by class
	// Source: https://www.dragonsfoot.org/forums/viewtopic.php?t=78913&start=120
	// - Fighters: 9 Hit Dice (levels 1-9), then +3 HP/level without Constitution bonus
	// - Priests: 9 Hit Dice (levels 1-9), then fixed HP/level without Constitution bonus
	// - Rogues/Wizards: 10 Hit Dice (levels 1-10), then fixed HP/level without Constitution bonus
	switch (playerClassState)
	{

	case PlayerClassState::FIGHTER:
	{
		return std::min(level, 9); // AD&D 2e: Fighters get Con bonus for 9 levels
	}

	case PlayerClassState::CLERIC:
	{
		return std::min(level, 9); // AD&D 2e: Priests get Con bonus for 9 levels
	}

	case PlayerClassState::ROGUE:
	case PlayerClassState::WIZARD:
	{
		return std::min(level, 10); // AD&D 2e: Rogues/Wizards get Con bonus for 10 levels
	}

	default:
	{
		return std::min(level, 10);
	}

	}
}

void Player::save(json& j)
{
	Creature::save(j); // Call base class save

	// Player-specific fields
	j["playerRaceState"] = static_cast<int>(playerRaceState);
	j["playerClassState"] = static_cast<int>(playerClassState);
	j["playerClass"] = playerClass;
	j["playerRace"] = playerRace;
	j["roundCounter"] = roundCounter;
	j["webStuckTurns"] = webStuckTurns;
	j["webStrength"] = webStrength;
	j["killCount"] = killCount;
	j["memorizedSpells"] = memorizedSpells;

	// Save equipped items
	json equippedJson = json::array();
	for (const auto& equipped : equippedItems)
	{
		json itemEntry;
		itemEntry["slot"] = static_cast<int>(equipped.slot);
		json itemJson;
		equipped.item->save(itemJson);
		itemEntry["item"] = itemJson;
		equippedJson.push_back(itemEntry);
	}
	j["equippedItems"] = equippedJson;
}

void Player::load(const json& j)
{
	Creature::load(j); // Call base class load
	// Creature::load constructs MonsterAttacker when attacker data is present.
	// Replace with the correct PlayerAttacker strategy.
	attacker = std::make_unique<PlayerAttacker>(*this);
	// PlayerController is not in the Ai hierarchy -- construct directly.
	controller = std::make_unique<PlayerController>(*this);

	// Player-specific fields
	playerRaceState = static_cast<PlayerRaceState>(j.at("playerRaceState").get<int>());
	playerClassState = static_cast<PlayerClassState>(j.at("playerClassState").get<int>());
	playerClass = j.at("playerClass").get<std::string>();
	playerRace = j.at("playerRace").get<std::string>();
	roundCounter = j.at("roundCounter").get<int>();
	webStuckTurns = j.at("webStuckTurns").get<int>();
	webStrength = j.at("webStrength").get<int>();
	killCount = j.value("killCount", 0);
	if (j.contains("memorizedSpells"))
	{
		memorizedSpells = j["memorizedSpells"].get<std::vector<std::string>>();
	}
	// trappingWeb is not serialized - it's a map reference that needs to be re-established

	// Load equipped items
	equippedItems.clear();
	if (j.contains("equippedItems"))
	{
		for (const auto& itemEntry : j["equippedItems"])
		{
			EquipmentSlot slot = static_cast<EquipmentSlot>(itemEntry.at("slot").get<int>());
			auto item = std::make_unique<Item>(Vector2D{ 0, 0 }, ActorData{ TileRef{}, "temp", 0 });
			item->load(itemEntry["item"]);
			equippedItems.emplace_back(std::move(item), slot);
		}
	}
}

void Player::remove_stat_bonuses_from_equipment(Item& item)
{
	if (!item.behavior)
	{
		return;
	}

	auto remove_stats = [this](auto& sb)
	{
		using T = std::decay_t<decltype(sb)>;
		if constexpr (std::is_same_v<T, JewelryAmulet> || std::is_same_v<T, Gauntlets> || std::is_same_v<T, Girdle>)
		{
			if (sb.isSetMode)
			{
				if (sb.strBonus != 0)
				{
					set_strength(sb.originalStats.str);
				}
				if (sb.dexBonus != 0)
				{
					set_dexterity(sb.originalStats.dex);
				}
				if (sb.conBonus != 0)
				{
					set_constitution(sb.originalStats.con);
				}
				if (sb.intBonus != 0)
				{
					set_intelligence(sb.originalStats.intel);
				}
				if (sb.wisBonus != 0)
				{
					set_wisdom(sb.originalStats.wis);
				}
				if (sb.chaBonus != 0)
				{
					set_charisma(sb.originalStats.cha);
				}
			}
			else
			{
				set_strength(get_strength() - sb.strBonus);
				set_dexterity(get_dexterity() - sb.dexBonus);
				set_constitution(get_constitution() - sb.conBonus);
				set_intelligence(get_intelligence() - sb.intBonus);
				set_wisdom(get_wisdom() - sb.wisBonus);
				set_charisma(get_charisma() - sb.chaBonus);
			}
		}
	};

	std::visit(remove_stats, *item.behavior);
}

void Player::update(GameContext& ctx)
{
	update_creature_state(ctx);
	assert(controller && "Player::update called with null controller");
	controller->update(ctx);
}

void Player::apply_confusion(int duration)
{
	assert(controller && "Player::apply_confusion called with null controller");
	controller->apply_confusion(duration);
}

int Player::get_next_level_xp(GameContext& ctx) const
{
	int currentLevel = get_creature_level();

	switch (playerClassState)
	{

	case PlayerClassState::FIGHTER:
	{
		return calculate_fighter_xp(currentLevel);
	}

	case PlayerClassState::ROGUE:
	{
		return calculate_rogue_xp(currentLevel);
	}

	case PlayerClassState::CLERIC:
	{
		return calculate_cleric_xp(currentLevel);
	}

	case PlayerClassState::WIZARD:
	{
		return calculate_wizard_xp(currentLevel);
	}

	default:
	{
		return 2000 * currentLevel;
	}

	}
}

void Player::levelup_update(GameContext& ctx)
{
	ctx.messageSystem->log("Player::levelup_update");
	int levelUpXp = get_next_level_xp(ctx);
	if (destructible->get_xp() >= levelUpXp)
	{
		adjust_level(1);
		destructible->set_xp(destructible->get_xp() - levelUpXp);
		ctx.messageSystem->message(
			WHITE_BLACK_PAIR,
			std::format("Your battle skills grow stronger! You reached level {}", get_creature_level()),
			true);

		if (ctx.displayManager != nullptr)
		{
			ctx.displayManager->display_levelup(*this, get_creature_level(), ctx);
		}
	}
}

// end of file: Player.cpp
