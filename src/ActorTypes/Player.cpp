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

			// Skip shopkeepers - they're not hostile
			if (dynamic_cast<AiShopkeeper*>(creature->ai.get()))
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
	game.appendMessagePart(WHITE_BLACK_PAIR, "Resting recovers ");
	game.appendMessagePart(WHITE_GREEN_PAIR, std::to_string(amountHealed));
	game.appendMessagePart(WHITE_GREEN_PAIR, " health");

	if (beforeState != afterState)
	{
		// If hunger state changed, mention it
		game.appendMessagePart(WHITE_BLACK_PAIR, ", but you've become ");
		game.appendMessagePart(game.hunger_system.get_hunger_color(), game.hunger_system.get_hunger_state_string().c_str());
		game.appendMessagePart(WHITE_BLACK_PAIR, ".");
	}
	else
	{
		game.appendMessagePart(WHITE_BLACK_PAIR, ", consuming your food.");
	}

	game.finalizeMessage();

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

void Player::getStuckInWeb(int duration, int strength, Web* web)
{
	webStuckTurns = duration;
	webStrength = strength;
	trappingWeb = web;

	game.message(WHITE_BLACK_PAIR, "You're caught in a sticky web for " +
		std::to_string(duration) + " turns!", true);
}

bool Player::tryBreakWeb()
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

bool Player::equipItem(std::unique_ptr<Item> item, EquipmentSlot slot, bool twoHanded)
{
	if (!item || !canEquip(*item, slot, twoHanded))
	{
		return false;
	}

	// Unequip existing item in the slot first
	unequipItem(slot);

	// For two-handed weapons, also unequip off-hand
	if (twoHanded && slot == EquipmentSlot::MAIN_HAND)
	{
		unequipItem(EquipmentSlot::OFF_HAND);
	}

	// Add equipped item
	equippedItems.emplace_back(std::move(item), slot, twoHanded);
	
	// Mark item as equipped
	equippedItems.back().item->add_state(ActorState::IS_EQUIPPED);
	
	// Update armor class if armor or shield was equipped
	if (slot == EquipmentSlot::ARMOR || slot == EquipmentSlot::OFF_HAND)
	{
		destructible->update_armor_class(*this);
		game.message(WHITE_BLACK_PAIR, "Your armor class is now " + std::to_string(destructible->armorClass) + ".", true);
	}
	
	return true;
}

// Equipment system implementation
bool Player::canEquip(const Item& item, EquipmentSlot slot, bool twoHanded) const
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

	// For two-handed weapons or when using versatile weapons two-handed
	if (twoHanded)
	{
		// Cannot equip two-handed if off-hand is occupied
		if (isSlotOccupied(EquipmentSlot::OFF_HAND))
		{
			return false;
		}
	}

	// For off-hand items
	if (slot == EquipmentSlot::OFF_HAND)
	{
		// Cannot equip off-hand if already using two-handed weapon
		if (hasEquippedTwoHandedWeapon())
		{
			return false;
		}
	}

	return true;
}

bool Player::unequipItem(EquipmentSlot slot)
{
	auto it = std::find_if(equippedItems.begin(), equippedItems.end(),
		[slot](const EquippedItem& equipped) { return equipped.slot == slot; });
	
	if (it != equippedItems.end())
	{
		// Remove equipped state
		it->item->remove_state(ActorState::IS_EQUIPPED);
		
		// Reset combat stats if main hand weapon
		if (slot == EquipmentSlot::MAIN_HAND)
		{
			attacker->roll = "D2"; // Reset to unarmed
			remove_state(ActorState::IS_RANGED); // Remove ranged state
		}
		
		// Return item to inventory
		container->add(std::move(it->item));
		
		// Remove from equipped items
		equippedItems.erase(it);
		
		// Update armor class if armor or shield was unequipped
		if (slot == EquipmentSlot::ARMOR || slot == EquipmentSlot::OFF_HAND)
		{
			destructible->update_armor_class(*this);
			game.message(WHITE_BLACK_PAIR, "Your armor class is now " + std::to_string(destructible->armorClass) + ".", true);
		}
		
		return true;
	}
	
	return false;
}

Item* Player::getEquippedItem(EquipmentSlot slot) const
{
	auto it = std::find_if(equippedItems.begin(), equippedItems.end(),
		[slot](const EquippedItem& equipped) { return equipped.slot == slot; });
	
	return (it != equippedItems.end()) ? it->item.get() : nullptr;
}

bool Player::isSlotOccupied(EquipmentSlot slot) const
{
	return getEquippedItem(slot) != nullptr;
}

bool Player::hasEquippedTwoHandedWeapon() const
{
	// Check main hand for two-handed weapon or versatile weapon used two-handed
	auto mainHandItem = getEquippedItem(EquipmentSlot::MAIN_HAND);
	if (!mainHandItem)
	{
		return false;
	}
	
	// Find the equipped item details
	auto it = std::find_if(equippedItems.begin(), equippedItems.end(),
		[](const EquippedItem& equipped) { return equipped.slot == EquipmentSlot::MAIN_HAND; });
	
	if (it != equippedItems.end())
	{
		// Check if it's being used two-handed
		return it->twoHandedGrip;
	}
	
	return false;
}

std::string Player::getEquippedWeaponDamageRoll() const
{
	auto mainHandWeapon = getEquippedItem(EquipmentSlot::MAIN_HAND);
	if (!mainHandWeapon)
	{
		return "D2"; // Unarmed damage
	}
	
	// Find if weapon is being used two-handed
	auto it = std::find_if(equippedItems.begin(), equippedItems.end(),
		[](const EquippedItem& equipped) { return equipped.slot == EquipmentSlot::MAIN_HAND; });
	
	bool twoHanded = (it != equippedItems.end()) ? it->twoHandedGrip : false;
	
	// Get damage roll based on weapon type
	if (auto* dagger = dynamic_cast<Dagger*>(mainHandWeapon->pickable.get()))
	{
		return dagger->roll;
	}
	else if (auto* shortSword = dynamic_cast<ShortSword*>(mainHandWeapon->pickable.get()))
	{
		return shortSword->roll;
	}
	else if (auto* longSword = dynamic_cast<LongSword*>(mainHandWeapon->pickable.get()))
	{
		// For versatile weapons, check if using two-handed
		return twoHanded ? "D10" : longSword->roll;
	}
	else if (auto* longbow = dynamic_cast<Longbow*>(mainHandWeapon->pickable.get()))
	{
		return longbow->roll;
	}
	else if (auto* staff = dynamic_cast<Staff*>(mainHandWeapon->pickable.get()))
	{
		// Staff is versatile: D6 one-handed, D8 two-handed
		return twoHanded ? "D8" : staff->roll;
	}
	else if (auto* greatsword = dynamic_cast<Greatsword*>(mainHandWeapon->pickable.get()))
	{
		return greatsword->roll; // D12
	}
	else if (auto* battleAxe = dynamic_cast<BattleAxe*>(mainHandWeapon->pickable.get()))
	{
		// Battle axe is versatile: D8 one-handed, D10 two-handed
		return twoHanded ? "D10" : battleAxe->roll;
	}
	else if (auto* greatAxe = dynamic_cast<GreatAxe*>(mainHandWeapon->pickable.get()))
	{
		return greatAxe->roll; // D12
	}
	else if (auto* warHammer = dynamic_cast<WarHammer*>(mainHandWeapon->pickable.get()))
	{
		// War hammer is versatile: D8 one-handed, D10 two-handed
		return twoHanded ? "D10" : warHammer->roll;
	}
	
	return "D2"; // Fallback for unknown weapons
}

// end of file: Player.cpp
