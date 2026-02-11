// file: Player.cpp
#include <curses.h>

#include "../Map/Map.h"
#include "../Ai/Ai.h"
#include "../Ai/AiPlayer.h"
#include "../ActorTypes/Player.h"
#include "../Actor/Actor.h"
#include "../Actor/InventoryOperations.h"
#include "../Random/RandomDice.h"
#include "../Colors/Colors.h"
#include "../ActorTypes/Healer.h"
#include "../Objects/Web.h"
#include "../dnd_tables/CalculatedTHAC0s.h"
#include "../Items/Items.h"
#include "../Items/Armor.h"
#include "../Items/ItemClassification.h"
#include "../Items/Jewelry.h"
#include "../Combat/WeaponDamageRegistry.h"
#include "../Ai/AiShopkeeper.h"
#include "../Core/GameContext.h"
#include "../Factories/ItemCreator.h"
#include "../Systems/MessageSystem.h"
#include "../Systems/HungerSystem.h"
#include "../Systems/RenderingManager.h"
#include "../Systems/SpellSystem.h"
#include "../Systems/BuffSystem.h"

using namespace InventoryOperations; // For clean function calls

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
}

Player::Player(Vector2D position) : Creature(position, ActorData{ '@', "Player", WHITE_BLACK_PAIR })
{
	set_gold(100); // Default starting gold (increased to 200 for fighters in MenuClass)
	ai = std::make_unique<AiPlayer>(); // Player AI, handles player input
}

void Player::roll_new_character(GameContext& ctx)
{
    // Rolling for stats
    auto roll3d6 = [&ctx]() { 
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

    attacker = std::make_unique<Attacker>(DamageValues::Unarmed());
    destructible = std::make_unique<PlayerDestructible>(
        playerHp, playerDr, "your corpse", playerXp, 0, playerAC
    );
}

 
void Player::equip_class_starting_gear(GameContext& ctx)
{
	switch (playerClassState)
	{
	case PlayerClassState::FIGHTER:
	{
		int startingGold = (ctx.dice->d4() + ctx.dice->d4() + ctx.dice->d4() + ctx.dice->d4() + ctx.dice->d4()) * 10;
		set_gold(startingGold);
		equip_item(ItemCreator::create(ItemId::PLATE_MAIL, position), EquipmentSlot::BODY, ctx);
		equip_item(ItemCreator::create(ItemId::LONG_SWORD, position), EquipmentSlot::RIGHT_HAND, ctx);
		equip_item(ItemCreator::create(ItemId::MEDIUM_SHIELD, position), EquipmentSlot::LEFT_HAND, ctx);
		// DEBUG: Add potions for testing
		InventoryOperations::add_item(*ctx.inventory_data, ItemCreator::create(ItemId::INVISIBILITY_POTION, position));
		InventoryOperations::add_item(*ctx.inventory_data, ItemCreator::create(ItemId::POTION_OF_GIANT_STRENGTH, position));
		InventoryOperations::add_item(*ctx.inventory_data, ItemCreator::create(ItemId::POTION_OF_FIRE_RESISTANCE, position));
		InventoryOperations::add_item(*ctx.inventory_data, ItemCreator::create(ItemId::POTION_OF_COLD_RESISTANCE, position));
		ctx.message_system->message(WHITE_BLACK_PAIR, "Fighter equipped with plate mail, long sword, and shield.", true);
		break;
	}
	case PlayerClassState::ROGUE:
	{
		int startingGold = (ctx.dice->d6() + ctx.dice->d6()) * 10;
		set_gold(startingGold);
		equip_item(ItemCreator::create(ItemId::LEATHER_ARMOR, position), EquipmentSlot::BODY, ctx);
		equip_item(ItemCreator::create(ItemId::DAGGER, position), EquipmentSlot::RIGHT_HAND, ctx);
		InventoryOperations::add_item(*ctx.inventory_data, ItemCreator::create(ItemId::INVISIBILITY_POTION, position));
		ctx.message_system->message(WHITE_BLACK_PAIR, "Rogue equipped with leather armor and dagger. Invisibility potion in inventory.", true);
		break;
	}
	case PlayerClassState::CLERIC:
	{
		int startingGold = (ctx.dice->d6() + ctx.dice->d6() + ctx.dice->d6()) * 10;
		set_gold(startingGold);
		equip_item(ItemCreator::create(ItemId::CHAIN_MAIL, position), EquipmentSlot::BODY, ctx);
		equip_item(ItemCreator::create(ItemId::MACE, position), EquipmentSlot::RIGHT_HAND, ctx);
		equip_item(ItemCreator::create(ItemId::MEDIUM_SHIELD, position), EquipmentSlot::LEFT_HAND, ctx);
		InventoryOperations::add_item(*ctx.inventory_data, ItemCreator::create(ItemId::HEALTH_POTION, position));
		SpellSystem::show_memorization_menu(*this, ctx);
		ctx.message_system->message(WHITE_BLACK_PAIR, "Cleric equipped with chain mail, mace, and shield. Spells memorized.", true);
		break;
	}
	case PlayerClassState::WIZARD:
	{
		int startingGold = (ctx.dice->d4() + ctx.dice->d4()) * 10;
		set_gold(startingGold);
		equip_item(ItemCreator::create(ItemId::STAFF, position), EquipmentSlot::RIGHT_HAND, ctx);
		InventoryOperations::add_item(*ctx.inventory_data, ItemCreator::create(ItemId::SCROLL_FIREBALL, position));
		InventoryOperations::add_item(*ctx.inventory_data, ItemCreator::create(ItemId::SCROLL_LIGHTNING, position));
		SpellSystem::show_memorization_menu(*this, ctx);
		ctx.message_system->message(WHITE_BLACK_PAIR, "Wizard equipped with staff. Attack scrolls and spells ready.", true);
		break;
	}
	default:
		break;
	}
}
void Player::racial_ability_adjustments()
{
	// switch player state
	switch (playerRaceState)
	{
	case Player::PlayerRaceState::HUMAN:
		break;
	case Player::PlayerRaceState::DWARF:
		// Create a temporary window for racial bonus message
		{
			WINDOW* racialWindow = newwin(7, 80, (LINES/2)-3, (COLS/2)-40);
			box(racialWindow, 0, 0);
			mvwprintw(racialWindow, 2, 2, "You got +1 to constitution and -1 to charisma for being a dwarf.");
			mvwprintw(racialWindow, 4, 2, "Press any key to continue...");
			wrefresh(racialWindow);
			getch();
			delwin(racialWindow);
		}

		adjust_constitution(1);
		adjust_charisma(-1);
		break;
	case Player::PlayerRaceState::ELF:
		// Create a temporary window for racial bonus message
		{
			WINDOW* racialWindow = newwin(7, 80, (LINES/2)-3, (COLS/2)-40);
			box(racialWindow, 0, 0);
			mvwprintw(racialWindow, 2, 2, "You got +1 to dexterity and -1 to constitution for being an elf.");
			mvwprintw(racialWindow, 4, 2, "Press any key to continue...");
			wrefresh(racialWindow);
			getch();
			delwin(racialWindow);
		}

		adjust_dexterity(1);
		adjust_constitution(-1);
		break;
	case Player::PlayerRaceState::GNOME:
		// Create a temporary window for racial bonus message
		{
			WINDOW* racialWindow = newwin(7, 80, (LINES/2)-3, (COLS/2)-40);
			box(racialWindow, 0, 0);
			mvwprintw(racialWindow, 2, 2, "You got +1 to intelligence and -1 to wisdom for being a gnome.");
			mvwprintw(racialWindow, 4, 2, "Press any key to continue...");
			wrefresh(racialWindow);
			getch();
			delwin(racialWindow);
		}

		adjust_intelligence(1);
		adjust_wisdom(-1);
		break;
	case Player::PlayerRaceState::HALFELF:
		break;
	case Player::PlayerRaceState::HALFLING:
		// Create a temporary window for racial bonus message
		{
			WINDOW* racialWindow = newwin(7, 80, (LINES/2)-3, (COLS/2)-40);
			box(racialWindow, 0, 0);
			mvwprintw(racialWindow, 2, 2, "You got +1 to dexterity and -1 to strength for being a halfling.");
			mvwprintw(racialWindow, 4, 2, "Press any key to continue...");
			wrefresh(racialWindow);
			getch();
			delwin(racialWindow);
		}

		adjust_dexterity(1);
		adjust_strength(-1);
		break;
	default:
		break;
	}
	
	// Clear screen after all racial bonuses - GUI won't interfere since it doesn't render during STARTUP
	clear();
	refresh();
}

void Player::calculate_thaco()
{
	// Static instance for efficiency - no need to recreate each time
	static constexpr CalculatedTHAC0s thaco_tables;
	
	switch (playerClassState)
	{
	case PlayerClassState::FIGHTER:
		destructible->set_thaco(thaco_tables.get_fighter(get_player_level()));
		break;
	case PlayerClassState::ROGUE:
		destructible->set_thaco(thaco_tables.get_rogue(get_player_level()));
		break;
	case PlayerClassState::CLERIC:
		destructible->set_thaco(thaco_tables.get_cleric(get_player_level()));
		break;
	case PlayerClassState::WIZARD:
		destructible->set_thaco(thaco_tables.get_wizard(get_player_level()));
		break;
	case PlayerClassState::NONE:
		break;
	default:
		break;
	}
}

void Player::consume_food(int nutrition, GameContext& ctx) {
	// Decrease hunger by nutrition value
	ctx.hunger_system->decrease_hunger(ctx, nutrition);
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
		ctx.message_system->message(WHITE_BLACK_PAIR, "You're already at full health.", true);
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
				ctx.message_system->message(WHITE_BLACK_PAIR, "You can't rest with enemies nearby!", true);
				return false;
			}
		}
	}

	// Check if player has enough food (hunger isn't too high)
	if (ctx.hunger_system->get_hunger_state() == HungerState::STARVING ||
		ctx.hunger_system->get_hunger_state() == HungerState::DYING)
	{
		ctx.message_system->message(WHITE_RED_PAIR, "You're too hungry to rest!", true);
		return false;
	}

	// Show resting animation
	animate_resting(ctx);

	// Rest - heal 20% of max HP
	int healAmount = std::max(1, destructible->get_max_hp() / 5);
	int amountHealed = destructible->heal(healAmount);

	// Capture the hunger state before and after
	HungerState beforeState = ctx.hunger_system->get_hunger_state();

	// Consume food (increase hunger)
	const int hungerCost = 50;
	ctx.hunger_system->increase_hunger(ctx, hungerCost);

	HungerState afterState = ctx.hunger_system->get_hunger_state();

	// Display message with more detail
	ctx.message_system->append_message_part(WHITE_BLACK_PAIR, "Resting recovers ");
	ctx.message_system->append_message_part(WHITE_GREEN_PAIR, std::to_string(amountHealed));
	ctx.message_system->append_message_part(WHITE_GREEN_PAIR, " health");

	if (beforeState != afterState)
	{
		// If hunger state changed, mention it
		ctx.message_system->append_message_part(WHITE_BLACK_PAIR, ", but you've become ");
		ctx.message_system->append_message_part(ctx.hunger_system->get_hunger_color(), ctx.hunger_system->get_hunger_state_string().c_str());
		ctx.message_system->append_message_part(WHITE_BLACK_PAIR, ".");
	}
	else
	{
		ctx.message_system->append_message_part(WHITE_BLACK_PAIR, ", consuming your food.");
	}

	ctx.message_system->finalize_message();

	// Memorize spells for casters
	if (playerClassState == PlayerClassState::CLERIC || playerClassState == PlayerClassState::WIZARD)
	{
		SpellSystem::show_memorization_menu(*this, ctx);
	}

	// Resting takes time
	*ctx.game_status = GameStatus::NEW_TURN;
	return true;
}

void Player::animate_resting(GameContext& ctx)
{
	const std::vector<char> restSymbols = { 'z', 'z', 'Z', 'Z', 'Z' };
	const int delay = 80; // milliseconds between frames

	for (int i = 0; i < static_cast<int>(restSymbols.size()); i++)
	{
		clear();
		ctx.rendering_manager->render(ctx);
		attron(COLOR_PAIR(WHITE_GREEN_PAIR));
		mvaddch(position.y - 1, position.x + i, restSymbols[i]);
		attroff(COLOR_PAIR(WHITE_GREEN_PAIR));
		refresh();
		napms(delay);
	}

	clear();
	ctx.rendering_manager->render(ctx);
	refresh();
}

bool Player::attempt_hide(GameContext& ctx)
{
	// Only rogues can hide
	if (playerClassState != PlayerClassState::ROGUE)
	{
		ctx.message_system->message(WHITE_BLACK_PAIR, "Only rogues can hide in shadows.", true);
		return false;
	}

	// Already invisible
	if (is_invisible())
	{
		ctx.message_system->message(WHITE_BLACK_PAIR, "You are already hidden.", true);
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
		ctx.message_system->message(RED_BLACK_PAIR, "You cannot hide while being observed!", true);
		return false;
	}

	// Success - hide duration based on level
	int hideDuration = 10 + get_player_level() * 2;
	ctx.buff_system->add_buff(*this, BuffType::INVISIBILITY, 0, hideDuration, false);
	ctx.message_system->message(CYAN_BLACK_PAIR, std::format("You melt into the shadows... (Hidden for {} turns)", hideDuration), true);
	return true;
}

void Player::get_stuck_in_web(int duration, int strength, Web* web, GameContext& ctx)
{
	webStuckTurns = duration;
	webStrength = strength;
	trappingWeb = web;

	ctx.message_system->message(WHITE_BLACK_PAIR, "You're caught in a sticky web for " +
		std::to_string(duration) + " turns!", true);
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
		ctx.message_system->message(WHITE_BLACK_PAIR, "You break free from the web!", true);

		// Destroy the web that trapped the player
		if (trappingWeb) {
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
		ctx.message_system->message(WHITE_BLACK_PAIR, "You finally break free from the web!", true);

		// Destroy the web that trapped the player
		if (trappingWeb) {
			trappingWeb->destroy(ctx);
			trappingWeb = nullptr;
		}

		webStuckTurns = 0;
		webStrength = 0;
		return true;
	}

	ctx.message_system->message(WHITE_BLACK_PAIR, "You're still stuck in the web. Turns remaining: " +
		std::to_string(webStuckTurns), true);
	return false;
}

// Clean Weapon Equipment System using Unique IDs
bool Player::toggle_weapon(uint64_t item_unique_id, EquipmentSlot preferred_slot, GameContext& ctx)
{
	// Check if weapon is already equipped
	if (is_item_equipped(item_unique_id))
	{
		// Find which slot and unequip
		for (auto slot : {EquipmentSlot::RIGHT_HAND, EquipmentSlot::LEFT_HAND, EquipmentSlot::MISSILE_WEAPON})
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
		Item* itemToEquip = find_item_by_id(inventory_data, item_unique_id);
		if (itemToEquip)
		{
			// Remove item from inventory
			auto result = remove_item_by_id(inventory_data, item_unique_id);
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
		auto result = remove_item_by_id(inventory_data, item_unique_id);
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
		auto result = remove_item_by_id(inventory_data, item_unique_id);
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
		if (ctx.message_system->is_debug_mode()) ctx.message_system->log("DEBUG: equip_item failed - null item");
		return false;
	}

	if (!can_equip(*item, slot))
	{
		if (ctx.message_system->is_debug_mode()) {
			ctx.message_system->log("DEBUG: can_equip failed for " + item->actorData.name);
			ctx.message_system->log("DEBUG: Item class: " + std::to_string(static_cast<int>(item->itemClass)));
			ctx.message_system->log("DEBUG: Is armor: " + std::string(item->is_armor() ? "true" : "false"));
			ctx.message_system->log("DEBUG: Slot: " + std::to_string(static_cast<int>(slot)));
			ctx.message_system->log("DEBUG: equip_item failed - can_equip returned false for " + item->actorData.name + " in slot " + std::to_string(static_cast<int>(slot)));
		}
		// Return item to inventory since we can't equip it
		InventoryOperations::add_item(inventory_data, std::move(item));
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
			ctx.message_system->message(WHITE_BLACK_PAIR, "You grip the " + item->actorData.name + " with both hands.", true);
		}
	}

	// Add equipped item
	equippedItems.emplace_back(std::move(item), slot);
	
	// Mark item as equipped
	equippedItems.back().item->add_state(ActorState::IS_EQUIPPED);
	
	// Log weapon equip
	if (slot == EquipmentSlot::RIGHT_HAND && equippedItems.back().item->is_weapon())
	{
		std::string weaponDamage = WeaponDamageRegistry::get_damage_roll(equippedItems.back().item->itemId);
		ctx.message_system->log("Equipped " + equippedItems.back().item->actorData.name + " - damage: " + weaponDamage);
	}
	
	// Update armor class if armor or shield was equipped
	if (slot == EquipmentSlot::BODY || slot == EquipmentSlot::LEFT_HAND)
	{
		destructible->update_armor_class(*this, ctx);
		ctx.message_system->message(WHITE_BLACK_PAIR, "Your armor class is now " + std::to_string(destructible->get_armor_class()) + ".", true);
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

	// Check if item has pickable component (weapons/armor)
	if (!item.pickable)
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
		// Other slots (CLOAK, BRACERS, BOOTS, MISSILES) - no items defined yet
		break;
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
		add_item(inventory_data, std::move(it->item));
		
		// Remove from equipped items
		equippedItems.erase(it);
		
		// Update armor class if armor or shield was unequipped
		if (slot == EquipmentSlot::BODY || slot == EquipmentSlot::LEFT_HAND)
		{
			destructible->update_armor_class(*this, ctx);
			ctx.message_system->message(WHITE_BLACK_PAIR, "Your armor class is now " + std::to_string(destructible->get_armor_class()) + ".", true);
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
		return WeaponDamageRegistry::get_damage_roll(rightHandWeapon->itemId);
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
			info.offHandDamageRoll = WeaponDamageRegistry::get_damage_roll(leftHandWeapon->itemId);
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
		auto result = remove_item_by_id(inventory_data, item_unique_id);
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
	const int level = get_player_level();

	// AD&D 2e: Constitution HP bonus per level caps by class
	// Source: https://www.dragonsfoot.org/forums/viewtopic.php?t=78913&start=120
	// - Fighters: 9 Hit Dice (levels 1-9), then +3 HP/level without Constitution bonus
	// - Priests: 9 Hit Dice (levels 1-9), then fixed HP/level without Constitution bonus
	// - Rogues/Wizards: 10 Hit Dice (levels 1-10), then fixed HP/level without Constitution bonus
	switch (playerClassState)
	{
		case PlayerClassState::FIGHTER:
			return std::min(level, 9);  // AD&D 2e: Fighters get Con bonus for 9 levels

		case PlayerClassState::CLERIC:
			return std::min(level, 9);  // AD&D 2e: Priests get Con bonus for 9 levels

		case PlayerClassState::ROGUE:
		case PlayerClassState::WIZARD:
			return std::min(level, 10); // AD&D 2e: Rogues/Wizards get Con bonus for 10 levels

		default:
			return std::min(level, 10);
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
	j["attacksPerRound"] = attacksPerRound;
	j["roundCounter"] = roundCounter;
	j["webStuckTurns"] = webStuckTurns;
	j["webStrength"] = webStrength;

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

	// Player-specific fields
	playerRaceState = static_cast<PlayerRaceState>(j.at("playerRaceState").get<int>());
	playerClassState = static_cast<PlayerClassState>(j.at("playerClassState").get<int>());
	playerClass = j.at("playerClass").get<std::string>();
	playerRace = j.at("playerRace").get<std::string>();
	attacksPerRound = j.at("attacksPerRound").get<float>();
	roundCounter = j.at("roundCounter").get<int>();
	webStuckTurns = j.at("webStuckTurns").get<int>();
	webStrength = j.at("webStrength").get<int>();
	// trappingWeb is not serialized - it's a map reference that needs to be re-established

	// Load equipped items
	equippedItems.clear();
	if (j.contains("equippedItems"))
	{
		for (const auto& itemEntry : j["equippedItems"])
		{
			EquipmentSlot slot = static_cast<EquipmentSlot>(itemEntry.at("slot").get<int>());
			auto item = std::make_unique<Item>(Vector2D{0, 0}, ActorData{' ', "temp", 0});
			item->load(itemEntry["item"]);
			equippedItems.emplace_back(std::move(item), slot);
		}
	}
}

void Player::remove_stat_bonuses_from_equipment(Item& item)
{
	if (!item.pickable)
		return;

	auto* statBoost = dynamic_cast<StatBoostEquipment*>(item.pickable.get());
	if (!statBoost)
		return;

	if (statBoost->is_set_mode)
	{
		if (statBoost->str_bonus != 0) set_strength(statBoost->original_stats.str);
		if (statBoost->dex_bonus != 0) set_dexterity(statBoost->original_stats.dex);
		if (statBoost->con_bonus != 0) set_constitution(statBoost->original_stats.con);
		if (statBoost->int_bonus != 0) set_intelligence(statBoost->original_stats.intel);
		if (statBoost->wis_bonus != 0) set_wisdom(statBoost->original_stats.wis);
		if (statBoost->cha_bonus != 0) set_charisma(statBoost->original_stats.cha);
	}
	else
	{
		set_strength(get_strength() - statBoost->str_bonus);
		set_dexterity(get_dexterity() - statBoost->dex_bonus);
		set_constitution(get_constitution() - statBoost->con_bonus);
		set_intelligence(get_intelligence() - statBoost->int_bonus);
		set_wisdom(get_wisdom() - statBoost->wis_bonus);
		set_charisma(get_charisma() - statBoost->cha_bonus);
	}
}

// end of file: Player.cpp
