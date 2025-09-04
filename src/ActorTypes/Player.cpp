// file: Player.cpp
#include <curses.h>
#include "../Game.h"
#include "../Map/Map.h"
#include "../Ai/Ai.h"
#include "../Ai/AiPlayer.h"
#include "../ActorTypes/Player.h"
#include "../Actor/Actor.h"
#include "../Random/RandomDice.h"
#include "../Colors/Colors.h"
#include "../ActorTypes/Healer.h"
#include "../Objects/Web.h"
#include "../dnd_tables/CalculatedTHAC0s.h"
#include "../Items/Items.h"
#include "../Items/Armor.h"
#include "../Utils/PickableTypeRegistry.h"
#include "../Ai/AiShopkeeper.h"

Player::Player(Vector2D position) : Creature(position, ActorData{ '@', "Player", WHITE_BLACK_PAIR })
{
	// Rolling for stats
	auto roll3d6 = []() { return game.d.d6() + game.d.d6() + game.d.d6(); };
	strength = roll3d6();
	dexterity = roll3d6();
	constitution = roll3d6();
	intelligence = roll3d6();
	wisdom = roll3d6();
	charisma = roll3d6();

	//==PLAYER==
	const int playerHp = 20 + game.d.d10(); // we roll the dice to get the player's hp
	const int playerDr = 1; // the player's damage reduction
	const int playerXp = 0; // the player's experience points
	const int playerAC = 10; // the player's armor class

	gold = 100; // Default starting gold (increased to 200 for fighters in MenuClass)
	attacker = std::make_unique<Attacker>("D2"); // Default attack roll, can be changed by equipping a weapon
	destructible = std::make_unique<PlayerDestructible>( // PlayerDestructible is a subclass of Destructible
		playerHp, // initial HP
		playerDr, // initial damage reduction
		"your corpse", // death message
		playerXp, // initial experience points
		0, // thaco is calculated from table
		playerAC // initial armor class
	);
	ai = std::make_unique<AiPlayer>(); // Player AI, handles player input
	container = std::make_unique<Container>(26); // Player's inventory with 26 slots
	
	// REMOVED: Equipment initialization moved to MenuClass after class selection
	
}

void Player::racial_ability_adjustments()
{
	// switch player state
	switch (game.player->playerRaceState)
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

		game.player->constitution += 1;
		game.player->charisma -= 1;
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

		game.player->dexterity += 1;
		game.player->constitution -= 1;
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

		game.player->intelligence += 1;
		game.player->wisdom -= 1;
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

		game.player->dexterity += 1;
		game.player->strength -= 1;
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
	// game.err("Calculating THAC0...");
	// print playerClassState

	CalculatedTHAC0s thaco;
	switch (playerClassState)
	{	
	case PlayerClassState::FIGHTER:
		game.player->destructible->thaco = thaco.getFighter(playerLevel);
		/*game.err(playerClass + "you got THAC0: " + std::to_string(game.player->destructible->thaco));*/
		break;
	case PlayerClassState::ROGUE:
		game.player->destructible->thaco = thaco.getRogue(playerLevel);
		/*game.err(playerClass + "you got THAC0: " + std::to_string(game.player->destructible->thaco));*/
		break;
	case PlayerClassState::CLERIC:
		game.player->destructible->thaco = thaco.getCleric(playerLevel);
		/*game.err(playerClass + "you got THAC0: " + std::to_string(game.player->destructible->thaco));*/
		break;
	case PlayerClassState::WIZARD:
		game.player->destructible->thaco = thaco.getWizard(playerLevel);
		/*game.err(playerClass + "you got THAC0: " + std::to_string(game.player->destructible->thaco));*/
		break;
	case PlayerClassState::NONE:
		break;
	default:
		break;
	}
}

void Player::consume_food(int nutrition) {
	// Decrease hunger by nutrition value
	game.hunger_system.decrease_hunger(nutrition);
}

void Player::render() const noexcept
{
	// First, render the player normally
	Creature::render();
}

bool Player::rest()
{
	// Check if player is already at full health
	if (destructible->hp >= destructible->hpMax)
	{
		game.message(WHITE_BLACK_PAIR, "You're already at full health.", true);
		return false;
	}

	// Check if enemies are nearby (within a radius of 5 tiles)
	// Exclude shopkeepers and other neutral NPCs
	for (const auto& creature : game.creatures)
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
				game.message(WHITE_BLACK_PAIR, "You can't rest with enemies nearby!", true);
				return false;
			}
		}
	}

	// Check if player has enough food (hunger isn't too high)
	if (game.hunger_system.get_hunger_state() == HungerState::STARVING ||
		game.hunger_system.get_hunger_state() == HungerState::DYING)
	{
		game.message(WHITE_RED_PAIR, "You're too hungry to rest!", true);
		return false;
	}

	// Show resting animation
	animate_resting();

	// Rest - heal 20% of max HP
	int healAmount = std::max(1, destructible->hpMax / 5);
	int amountHealed = destructible->heal(healAmount);

	// Capture the hunger state before and after
	HungerState beforeState = game.hunger_system.get_hunger_state();

	// Consume food (increase hunger)
	const int hungerCost = 50;
	game.hunger_system.increase_hunger(hungerCost);

	HungerState afterState = game.hunger_system.get_hunger_state();

	// Display message with more detail
	game.append_message_part(WHITE_BLACK_PAIR, "Resting recovers ");
	game.append_message_part(WHITE_GREEN_PAIR, std::to_string(amountHealed));
	game.append_message_part(WHITE_GREEN_PAIR, " health");

	if (beforeState != afterState)
	{
		// If hunger state changed, mention it
		game.append_message_part(WHITE_BLACK_PAIR, ", but you've become ");
		game.append_message_part(game.hunger_system.get_hunger_color(), game.hunger_system.get_hunger_state_string().c_str());
		game.append_message_part(WHITE_BLACK_PAIR, ".");
	}
	else
	{
		game.append_message_part(WHITE_BLACK_PAIR, ", consuming your food.");
	}

	game.finalize_message();

	// Resting takes time
	game.gameStatus = Game::GameStatus::NEW_TURN;
	return true;
}

void Player::animate_resting()
{
	// Animation frames - Z's of increasing size
	const std::vector<char> restSymbols = { 'z', 'Z', 'Z' };
	const int frames = 3;
	const int delay = 250; // milliseconds between frames

	// Save original screen
	clear();
	game.render();

	// Show animation
	for (int i = 0; i < frames; i++)
	{
		// Draw Zs above player
		attron(COLOR_PAIR(WHITE_GREEN_PAIR));
		mvaddch(position.y - 1, position.x + i, restSymbols[i]);
		attroff(COLOR_PAIR(WHITE_GREEN_PAIR));

		refresh();
		napms(delay);
	}

	// Pause briefly to show final frame
	napms(delay * 2);

	// Redraw screen
	clear();
	game.render();
	refresh();
}

void Player::get_stuck_in_web(int duration, int strength, Web* web)
{
	webStuckTurns = duration;
	webStrength = strength;
	trappingWeb = web;

	game.message(WHITE_BLACK_PAIR, "You're caught in a sticky web for " +
		std::to_string(duration) + " turns!", true);
}

bool Player::try_break_web()
{
	// Calculate chance to break free based on strength vs web strength
	int breakChance = 20 + (strength * 5) - (webStrength * 10);

	// Ensure some minimum chance
	breakChance = std::max(10, breakChance);

	// Roll to break free
	if (game.d.d100() <= breakChance)
	{
		// Success!
		game.message(WHITE_BLACK_PAIR, "You break free from the web!", true);

		// Destroy the web that trapped the player
		if (trappingWeb) {
			trappingWeb->destroy();
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
		game.message(WHITE_BLACK_PAIR, "You finally break free from the web!", true);

		// Destroy the web that trapped the player
		if (trappingWeb) {
			trappingWeb->destroy();
			trappingWeb = nullptr;
		}

		webStuckTurns = 0;
		webStrength = 0;
		return true;
	}

	game.message(WHITE_BLACK_PAIR, "You're still stuck in the web. Turns remaining: " +
		std::to_string(webStuckTurns), true);
	return false;
}

// Clean Weapon Equipment System using Unique IDs
bool Player::toggle_weapon(uint32_t item_unique_id, EquipmentSlot preferred_slot)
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
				return unequip_item(slot);
			}
		}
	}
	else
	{
		// Find item in inventory and equip it
		for (auto it = container->get_inventory_mutable().begin(); it != container->get_inventory_mutable().end(); ++it)
		{
			if ((*it)->uniqueId == item_unique_id)
			{
				// Move item from inventory to equipment
				auto itemToEquip = std::move(*it);
				container->get_inventory_mutable().erase(it);
				
				// Determine appropriate slot based on weapon type
				auto itemType = PickableTypeRegistry::get_item_type(*itemToEquip);
				EquipmentSlot target_slot;
				
				if (itemType == PickableTypeRegistry::Type::LONGBOW)
				{
					target_slot = EquipmentSlot::MISSILE_WEAPON;
				}
				else
				{
					target_slot = preferred_slot;
				}
				
				return equip_item(std::move(itemToEquip), target_slot);
			}
		}
	}
	return false;
}

bool Player::toggle_shield(uint32_t item_unique_id)
{
	// Shields always go in LEFT_HAND slot
	if (is_item_equipped(item_unique_id))
	{
		return unequip_item(EquipmentSlot::LEFT_HAND);
	}
	else
	{
		// Find item in inventory and equip it
		for (auto it = container->get_inventory_mutable().begin(); it != container->get_inventory_mutable().end(); ++it)
		{
			if ((*it)->uniqueId == item_unique_id)
			{
				// Move item from inventory to equipment
				auto itemToEquip = std::move(*it);
				container->get_inventory_mutable().erase(it);
				
				return equip_item(std::move(itemToEquip), EquipmentSlot::LEFT_HAND);
			}
		}
		return false;
	}
}

bool Player::equip_item(std::unique_ptr<Item> item, EquipmentSlot slot)
{
	if (!item || !can_equip(*item, slot))
	{
		return false;
	}

	// Unequip existing item in the slot first
	unequip_item(slot);
	
	// Special handling for two-handed weapons - use type registry
	if (slot == EquipmentSlot::RIGHT_HAND)
	{
		auto itemType = PickableTypeRegistry::get_item_type(*item);
		if (itemType == PickableTypeRegistry::Type::GREATSWORD || itemType == PickableTypeRegistry::Type::GREAT_AXE || itemType == PickableTypeRegistry::Type::LONGBOW)
		{
			// Two-handed weapon - also unequip left hand
			unequip_item(EquipmentSlot::LEFT_HAND);
			game.message(WHITE_BLACK_PAIR, "You grip the " + item->actorData.name + " with both hands.", true);
		}
	}

	// Add equipped item
	equippedItems.emplace_back(std::move(item), slot);
	
	// Mark item as equipped
	equippedItems.back().item->add_state(ActorState::IS_EQUIPPED);
	
	// Update armor class if armor or shield was equipped
	if (slot == EquipmentSlot::BODY || slot == EquipmentSlot::LEFT_HAND)
	{
		destructible->update_armor_class(*this);
		game.message(WHITE_BLACK_PAIR, "Your armor class is now " + std::to_string(destructible->armorClass) + ".", true);
	}
	
	return true;
}

// Equipment system implementation
bool Player::can_equip(const Item& item, EquipmentSlot slot) const
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
		// Hand slots can hold weapons or shields - use type registry
		auto itemType = PickableTypeRegistry::get_item_type(item);
		bool isWeapon = PickableTypeRegistry::is_weapon(itemType);
		bool isShield = (itemType == PickableTypeRegistry::Type::SHIELD);
		
		if (!isWeapon && !isShield)
		{
			return false; // Not a weapon or shield
		}
		
		// Check for two-handed weapon restrictions
		if (isWeapon)
		{
			// Two-handed weapons can only go in RIGHT_HAND
			if (itemType == PickableTypeRegistry::Type::GREATSWORD || itemType == PickableTypeRegistry::Type::GREAT_AXE || itemType == PickableTypeRegistry::Type::LONGBOW)
			{
				if (slot != EquipmentSlot::RIGHT_HAND)
				{
					return false; // Two-handed weapons must go in right hand
				}
			}
			
			// Check if trying to equip one-handed weapon when two-handed weapon is equipped
			if (slot == EquipmentSlot::LEFT_HAND)
			{
				auto* rightHandItem = get_equipped_item(EquipmentSlot::RIGHT_HAND);
				if (rightHandItem)
				{
					auto rightType = PickableTypeRegistry::get_item_type(*rightHandItem);
					if (rightType == PickableTypeRegistry::Type::GREATSWORD || rightType == PickableTypeRegistry::Type::GREAT_AXE || rightType == PickableTypeRegistry::Type::LONGBOW)
					{
						return false; // Can't equip anything in left hand when two-handed weapon equipped
					}
				}
				
				// Check if this weapon can be used in off-hand (must be TINY or SMALL)
				auto weaponSize = PickableTypeRegistry::get_weapon_size(itemType);
				if (weaponSize > WeaponSize::SMALL)
				{
					return false; // Weapon too large for off-hand
				}
			}
		}
		
		// Shields can only go in left hand
		if (isShield && slot != EquipmentSlot::LEFT_HAND)
		{
			return false;
		}
		
		break;
	}
	case EquipmentSlot::BODY:
	{
		// Body slot can only hold armor - use type registry
		auto itemType = PickableTypeRegistry::get_item_type(item);
		if (!PickableTypeRegistry::is_armor(itemType))
		{
			return false;
		}
		break;
	}
	case EquipmentSlot::MISSILE_WEAPON:
	{
		// Missile weapon slot can only hold ranged weapons - use type registry
		auto itemType = PickableTypeRegistry::get_item_type(item);
		if (!PickableTypeRegistry::is_ranged_weapon(itemType))
		{
			return false;
		}
		break;
	}
	default:
		// TODO: Add validation for other slots (HEAD, NECK, etc.)
		// For now, allow any item in other slots
		break;
	}

	return true;
}

bool Player::unequip_item(EquipmentSlot slot)
{
	auto it = std::find_if(equippedItems.begin(), equippedItems.end(),
		[slot](const EquippedItem& equipped) { return equipped.slot == slot; });
	
	if (it != equippedItems.end())
	{
		// Remove equipped state
		it->item->remove_state(ActorState::IS_EQUIPPED);
		
		// Reset combat stats if main hand weapon
		if (slot == EquipmentSlot::RIGHT_HAND)
		{
			attacker->roll = "D2"; // Reset to unarmed
			remove_state(ActorState::IS_RANGED); // Remove ranged state
		}
		
		// Return item to inventory
		container->add(std::move(it->item));
		
		// Remove from equipped items
		equippedItems.erase(it);
		
		// Update armor class if armor or shield was unequipped
		if (slot == EquipmentSlot::BODY || slot == EquipmentSlot::LEFT_HAND)
		{
			destructible->update_armor_class(*this);
			game.message(WHITE_BLACK_PAIR, "Your armor class is now " + std::to_string(destructible->armorClass) + ".", true);
		}
		
		return true;
	}
	
	return false;
}

Item* Player::get_equipped_item(EquipmentSlot slot) const
{
	auto it = std::find_if(equippedItems.begin(), equippedItems.end(),
		[slot](const EquippedItem& equipped) { return equipped.slot == slot; });
	
	return (it != equippedItems.end()) ? it->item.get() : nullptr;
}

bool Player::is_slot_occupied(EquipmentSlot slot) const
{
	return get_equipped_item(slot) != nullptr;
}

bool Player::is_dual_wielding() const
{
	// Check if both hands have weapons equipped
	auto rightHand = get_equipped_item(EquipmentSlot::RIGHT_HAND);
	auto leftHand = get_equipped_item(EquipmentSlot::LEFT_HAND);
	
	if (!rightHand || !leftHand)
	{
		return false;
	}
	
	// Check if left hand item is a weapon (not a shield) - use type registry
	auto leftType = PickableTypeRegistry::get_item_type(*leftHand);
	if (leftType == PickableTypeRegistry::Type::SHIELD)
	{
		return false; // Shield, not dual wielding
	}
	
	// Check if left hand item is a weapon
	if (PickableTypeRegistry::is_weapon(leftType))
	{
		return true; // Both hands have weapons
	}
	
	return false;
}

std::string Player::get_equipped_weapon_damage_roll() const
{
	auto rightHandWeapon = get_equipped_item(EquipmentSlot::RIGHT_HAND);
	if (!rightHandWeapon)
	{
		return "D2"; // Unarmed damage
	}
	
	// Get damage roll from the weapon's roll property - use type registry
	auto weaponType = PickableTypeRegistry::get_item_type(*rightHandWeapon);
	if (PickableTypeRegistry::is_weapon(weaponType))
	{
		// Access weapon roll through pickable interface - weapons are all Weapon subclasses
		if (auto* weapon = static_cast<Weapon*>(rightHandWeapon->pickable.get()))
		{
			return weapon->roll; // All weapons have a roll property
		}
	}
	
	return "D2"; // Fallback for non-weapons
}

Player::DualWieldInfo Player::get_dual_wield_info() const
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
	
	// TODO: Add proficiency check to reduce penalties
	// If player has Two-Weapon Fighting proficiency:
	// - Main hand penalty becomes 0
	// - Off-hand penalty becomes -2
	
	// Get off-hand weapon damage roll - use type registry
	auto leftHandWeapon = get_equipped_item(EquipmentSlot::LEFT_HAND);
	if (leftHandWeapon)
	{
		// Verify it's a weapon using type registry, then access roll property
		auto leftType = PickableTypeRegistry::get_item_type(*leftHandWeapon);
		if (PickableTypeRegistry::is_weapon(leftType))
		{
			if (auto* weapon = static_cast<Weapon*>(leftHandWeapon->pickable.get()))
			{
				info.offHandDamageRoll = weapon->roll;
			}
		}
	}
	
	return info;
}

// Clean Equipment System using Unique IDs
bool Player::toggle_armor(uint32_t item_unique_id)
{
	// Check if item is already equipped
	if (is_item_equipped(item_unique_id))
	{
		// Unequip the armor
		return unequip_item(EquipmentSlot::BODY);
	}
	else
	{
		// Find item in inventory and equip it
		for (auto it = container->get_inventory_mutable().begin(); it != container->get_inventory_mutable().end(); ++it)
		{
			if ((*it)->uniqueId == item_unique_id)
			{
				// Move item from inventory to equipment
				auto itemToEquip = std::move(*it);
				container->get_inventory_mutable().erase(it);
				
				return equip_item(std::move(itemToEquip), EquipmentSlot::BODY);
			}
		}
		return false; // Item not found in inventory
	}
}

bool Player::is_item_equipped(uint32_t item_unique_id) const
{
	for (const auto& equipped : equippedItems)
	{
		if (equipped.item->uniqueId == item_unique_id)
		{
			return true;
		}
	}
	return false;
}

// end of file: Player.cpp
