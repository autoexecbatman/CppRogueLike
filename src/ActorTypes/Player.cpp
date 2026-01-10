// file: Player.cpp
#include <curses.h>
#include "../Game.h"
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
#include "../Combat/WeaponDamageRegistry.h"
#include "../Ai/AiShopkeeper.h"

using namespace InventoryOperations; // For clean function calls

Player::Player(Vector2D position) : Creature(position, ActorData{ '@', "Player", WHITE_BLACK_PAIR })
{
	// Rolling for stats
	auto roll3d6 = []() { return game.d.d6() + game.d.d6() + game.d.d6(); };
	set_strength(roll3d6());
	set_dexterity(roll3d6());
	set_constitution(roll3d6());
	set_intelligence(roll3d6());
	set_wisdom(roll3d6());
	set_charisma(roll3d6());

	//==PLAYER==
	const int playerHp = 20 + game.d.d10(); // we roll the dice to get the player's hp
	const int playerDr = 1; // the player's damage reduction
	const int playerXp = 0; // the player's experience points
	const int playerAC = 10; // the player's armor class

	set_gold(100); // Default starting gold (increased to 200 for fighters in MenuClass)
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
	// inventory_data is initialized in Creature constructor
	
	// REMOVED: Equipment initialization moved to MenuClass after class selection
	
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

	// Resting takes time
	ctx.game->gameStatus = Game::GameStatus::NEW_TURN;
	return true;
}

void Player::animate_resting(GameContext& ctx)
{
	// Animation frames - Z's of increasing size
	const std::vector<char> restSymbols = { 'z', 'Z', 'Z' };
	const int frames = 3;
	const int delay = 250; // milliseconds between frames

	// Save original screen
	clear();
	ctx.game->render();

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
	ctx.game->render();
	refresh();
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
		ctx.message_system->message(WHITE_BLACK_PAIR, "You finally break free from the web!", true);

		// Destroy the web that trapped the player
		if (trappingWeb) {
			trappingWeb->destroy();
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
				
				if (itemToEquip->itemClass == ItemClass::LONG_BOW)
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
		for (auto it = inventory_data.items.begin(); it != inventory_data.items.end(); ++it)
		{
			if ((*it)->uniqueId == item_unique_id)
			{
				// Move item from inventory
				auto itemToEquip = std::move(*it);
				inventory_data.items.erase(it);
				
				return equip_item(std::move(itemToEquip), EquipmentSlot::LEFT_HAND);
			}
		}
		return false;
	}
}

bool Player::equip_item(std::unique_ptr<Item> item, EquipmentSlot slot)
{
	auto ctx = game.get_context();
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
	unequip_item(slot);
	
	// Special handling for two-handed weapons - use ItemClass system
	if (slot == EquipmentSlot::RIGHT_HAND)
	{
		if (item->is_two_handed_weapon())
		{
			// Two-handed weapon - also unequip left hand
			unequip_item(EquipmentSlot::LEFT_HAND);
			ctx.message_system->message(WHITE_BLACK_PAIR, "You grip the " + item->actorData.name + " with both hands.", true);
		}
	}

	// Add equipped item
	equippedItems.emplace_back(std::move(item), slot);
	
	// Mark item as equipped
	equippedItems.back().item->add_state(ActorState::IS_EQUIPPED);
	
	// Update weapon damage if it's a weapon
	if (slot == EquipmentSlot::RIGHT_HAND)
	{
		if (equippedItems.back().item->is_weapon() && attacker)
		{
			std::string weaponDamage = WeaponDamageRegistry::get_damage_roll(equippedItems.back().item->itemClass);
			attacker->set_roll(weaponDamage);
			ctx.message_system->log("Equipped " + equippedItems.back().item->actorData.name + " - damage updated to " + weaponDamage);
		}
	}
	
	// Update armor class if armor or shield was equipped
	if (slot == EquipmentSlot::BODY || slot == EquipmentSlot::LEFT_HAND)
	{
		destructible->update_armor_class(*this, ctx);
		ctx.message_system->message(WHITE_BLACK_PAIR, "Your armor class is now " + std::to_string(destructible->get_armor_class()) + ".", true);
	}
	
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
	default:
		// TODO: Add validation for other slots (HEAD, NECK, etc.)
		// For now, allow any item in other slots
		break;
	}

	return true;
}

bool Player::unequip_item(EquipmentSlot slot)
{
	auto ctx = game.get_context();
	auto it = std::find_if(equippedItems.begin(), equippedItems.end(),
		[slot](const EquippedItem& equipped) { return equipped.slot == slot; });
	
	if (it != equippedItems.end())
	{
		// Remove equipped state
		it->item->remove_state(ActorState::IS_EQUIPPED);
		
		// Reset combat stats if main hand weapon
		if (slot == EquipmentSlot::RIGHT_HAND)
		{
			if (it->item->is_weapon() && attacker)
			{
				attacker->set_roll(WeaponDamageRegistry::get_unarmed_damage());
				ctx.message_system->log("Unequipped " + it->item->actorData.name + " - damage reset to " + WeaponDamageRegistry::get_unarmed_damage());
			}
			remove_state(ActorState::IS_RANGED); // Remove ranged state
		}
		
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
	auto it = std::find_if(equippedItems.begin(), equippedItems.end(),
		[slot](const EquippedItem& equipped) { return equipped.slot == slot; });
	
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
		return WeaponDamageRegistry::get_damage_roll(rightHandWeapon->itemClass);
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
	
	// TODO: Add proficiency check to reduce penalties
	// If player has Two-Weapon Fighting proficiency:
	// - Main hand penalty becomes 0
	// - Off-hand penalty becomes -2
	
	// Get off-hand weapon damage roll - use pure ItemClass system
	auto leftHandWeapon = get_equipped_item(EquipmentSlot::LEFT_HAND);
	if (leftHandWeapon)
	{
		if (leftHandWeapon->is_weapon())
		{
			info.offHandDamageRoll = WeaponDamageRegistry::get_damage_roll(leftHandWeapon->itemClass);
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
		for (auto it = inventory_data.items.begin(); it != inventory_data.items.end(); ++it)
		{
			if ((*it)->uniqueId == item_unique_id)
			{
				// Move item from inventory
				auto itemToEquip = std::move(*it);
				inventory_data.items.erase(it);
				
				return equip_item(std::move(itemToEquip), EquipmentSlot::BODY);
			}
		}
		return false; // Item not found in inventory
	}
}

bool Player::is_item_equipped(uint32_t item_unique_id) const noexcept
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
