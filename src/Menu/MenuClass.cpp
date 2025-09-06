// file: MenuClass.cpp
#include "MenuClass.h"
#include "MenuName.h"
#include "../Game.h"
#include "../Random/RandomDice.h"
#include "../ActorTypes/Player.h"
#include "../Items/Items.h"
#include "../Items/Armor.h"
#include "../Actor/Pickable.h"
#include "../ActorTypes/Healer.h"
#include "../Factories/ItemCreator.h"
#include "../Utils/ItemTypeUtils.h"
#include "../Utils/PickableTypeRegistry.h"

void equip_fighter_starting_gear()
{
	auto& player = *game.player;
	
	// AD&D 2e Fighter starting money: 5d4 × 10 gp (50-200 gp)
	int rollSum = 0;
	for(int i = 0; i < 5; i++)
	{
		rollSum += game.d.d4(); // Use game's RandomDice instance
	}
	int startingGold = rollSum * 10;
	player.set_gold(startingGold);
	
	// Generous starting equipment for solo play
	
	// PLATE MAIL (AC 3) - Add to inventory
	auto plate_mail = ItemCreator::create_plate_mail(player.position);
	auto plate_mail_id = plate_mail->uniqueId; // Store ID before moving
	game.log("Created item with name: '" + plate_mail->actorData.name + "' and ID: " + std::to_string(plate_mail_id));
	game.log("Created plate mail with ID: " + std::to_string(plate_mail_id));
	auto plate_mail_result = player.container->add(std::move(plate_mail));
	if (plate_mail_result.has_value())
	{
		game.log("Plate mail added to inventory successfully");
		// Find plate mail by unique ID and equip it
		auto item_to_equip = ItemTypeUtils::find_item_by_id(player.container->get_inventory_mutable(), plate_mail_id);
		if (item_to_equip)
		{
			game.log("Found plate mail in inventory, attempting to equip");
			// Extract item from inventory for equipment
			auto extracted_item = ItemTypeUtils::extract_item_by_id(*player.container, plate_mail_id);
			if (extracted_item)
			{
				game.log("DEBUG: Extracted item for equipment, proceeding to auto-detect slot");
				
				// Auto-detect correct slot based on proper item classification
				std::string itemName = extracted_item->actorData.name;
				game.log("DEBUG: Checking item class: " + std::to_string(static_cast<int>(extracted_item->itemClass)));
				
				EquipmentSlot targetSlot;
				
				// Use proper item classification system instead of name parsing
				if (extracted_item->is_armor())
				{
					targetSlot = EquipmentSlot::BODY;
					game.log("DEBUG: Detected as armor by classification, targeting BODY slot");
				}
				else if (extracted_item->is_shield())
				{
					targetSlot = EquipmentSlot::LEFT_HAND;
					game.log("DEBUG: Detected as shield by classification, targeting LEFT_HAND slot");
				}
				else
				{
					game.log("ERROR: Unknown armor/shield type for " + itemName);
					player.container->add(std::move(extracted_item));
					return;
				}
				
				bool equipped = player.equip_item(std::move(extracted_item), targetSlot);
				if (equipped)
				{
					game.log("SUCCESS: " + itemName + " equipped to correct slot");
				}
				else
				{
					game.log("ERROR: Failed to equip " + itemName + " - item validation failed or disappeared");
				}
			}
		}
	}
	else
	{
		game.log("ERROR: Failed to add plate mail to inventory");
	}

	// LONG SWORD (1d8) - Add to inventory  
	auto long_sword = ItemCreator::create_long_sword(player.position);
	auto long_sword_id = long_sword->uniqueId; // Store ID before moving
	game.log("Created item with name: '" + long_sword->actorData.name + "' and ID: " + std::to_string(long_sword_id));
	game.log("Created long sword with ID: " + std::to_string(long_sword_id));
	auto sword_result = player.container->add(std::move(long_sword));
	if (sword_result.has_value())
	{
		game.log("Long sword added to inventory successfully");
		// Find long sword by unique ID and equip it
		auto item_to_equip = ItemTypeUtils::find_item_by_id(player.container->get_inventory_mutable(), long_sword_id);
		if (item_to_equip)
		{
			game.log("Found long sword in inventory, attempting to equip");
			// Extract item from inventory for equipment
			auto extracted_item = ItemTypeUtils::extract_item_by_id(*player.container, long_sword_id);
			if (extracted_item)
			{
				// Store item name for logging before potential move
				std::string itemName = extracted_item->actorData.name;
				bool equipped = player.equip_item(std::move(extracted_item), EquipmentSlot::RIGHT_HAND);
				if (equipped)
				{
					game.log("SUCCESS: " + itemName + " equipped to RIGHT_HAND slot");
					game.log("Current weapon damage: " + player.attacker->get_roll());
				}
				else
				{
					game.log("ERROR: Failed to equip " + itemName + " - item validation failed or disappeared");
				}
			}
		}
		else
		{
			game.log("ERROR: Could not find long sword in inventory after adding");
		}
	}
	else
	{
		game.log("ERROR: Failed to add long sword to inventory");
	}
	
	// Sync ranged state and update combat stats from equipped weapons
	player.sync_ranged_state();
	
	// Update AC based on equipped armor (proper way)
	player.destructible->update_armor_class(player);
	
	// HEALING POTIONS (3x)
	for(int i = 0; i < 3; i++)
	{
		player.container->add(ItemCreator::create_health_potion(player.position));
	}
	
	// Log the rolled starting gold amount
	game.log("Fighter starting gold: " + std::to_string(startingGold) + " gp (5d4×10)");
	game.message(WHITE_BLACK_PAIR, "You have " + std::to_string(startingGold) + " gold pieces.", true);
}

void Fighter::on_selection()
{
	game.player->playerClass = "Fighter";
	game.player->playerClassState = Player::PlayerClassState::FIGHTER;
	
	// EQUIP FIGHTER STARTING GEAR
	equip_fighter_starting_gear();
}

void Rogue::on_selection()
{
	game.player->playerClass = "Rogue";
	game.player->playerClassState = Player::PlayerClassState::ROGUE;
}

void Cleric::on_selection()
{
	game.player->playerClass = "Cleric";
	game.player->playerClassState = Player::PlayerClassState::CLERIC;
}

void Wizard::on_selection()
{
	game.player->playerClass = "Wizard";
	game.player->playerClassState = Player::PlayerClassState::WIZARD;
}

void ClassRandom::on_selection()
{
	RandomDice d;
	const int rng = d.d4();
	switch (rng)
	{
	case 1:
		game.player->playerClass = "Fighter";
		game.player->playerClassState = Player::PlayerClassState::FIGHTER;
		equip_fighter_starting_gear();
		break;
	case 2:
		game.player->playerClass = "Rogue";
		game.player->playerClassState = Player::PlayerClassState::ROGUE;
		break;
	case 3:
		game.player->playerClass = "Wizard";
		game.player->playerClassState = Player::PlayerClassState::WIZARD;
		break;
	case 4:
		game.player->playerClass = "Cleric";
		game.player->playerClassState = Player::PlayerClassState::CLERIC;
		break;
	default:break;
	}
}

void ClassBack::on_selection()
{
	game.menus.back()->back = true;
}

MenuClass::MenuClass()
{
	menu_new(menu_height, menu_width, menu_starty, menu_startx);
	iMenuStates.emplace(MenuState::FIGHTER, std::make_unique<Fighter>());
	iMenuStates.emplace(MenuState::ROGUE, std::make_unique<Rogue>());
	iMenuStates.emplace(MenuState::CLERIC, std::make_unique<Cleric>());
	iMenuStates.emplace(MenuState::WIZARD, std::make_unique<Wizard>());
	iMenuStates.emplace(MenuState::RANDOM, std::make_unique<ClassRandom>());
	iMenuStates.emplace(MenuState::BACK, std::make_unique<ClassBack>());
}

MenuClass::~MenuClass()
{
	menu_delete();
}

void MenuClass::menu_class_print_option(MenuState option) noexcept
{
	auto row = static_cast<int>(option) + 1; // Start at row 1 after title
	if (currentState == option)
	{
		menu_highlight_on();
	}
	menu_print(1, row, menu_class_get_string(option));
	if (currentState == option)
	{
		menu_highlight_off();
	}
}

void MenuClass::draw()
{
	menu_clear();
	box(menuWindow, 0, 0);
	// Title
	mvwprintw(menuWindow, 0, 1, "Select Class");
	for (size_t i{ 0 }; i < menuClassStrings.size(); ++i)
	{
		menu_class_print_option(static_cast<MenuState>(i));
	}
	menu_refresh();
}

void MenuClass::on_key(int key)
{
	switch (keyPress)
	{

	case KEY_UP:
	case 'w':
	{
		currentState = static_cast<MenuState>((static_cast<size_t>(currentState) + iMenuStates.size() - 1) % iMenuStates.size());
		break;
	}

	case KEY_DOWN:
	case 's':
	{
		currentState = static_cast<MenuState>((static_cast<size_t>(currentState) + iMenuStates.size() + 1) % iMenuStates.size());
		break;
	}

	case 10: // enter
	{
		menu_set_run_false();
		iMenuStates.at(currentState)->on_selection();
		if (currentState != MenuState::BACK)
		{
			MenuName menuName;
			menuName.menu_name();
		}
		break;
	}

	case 27: // escape
	{
		break;
	}

	default:
		break;
	}
}

void MenuClass::menu()
{
	while (run)
	{
		draw();
		menu_key_listen();
		on_key(keyPress);
	}
	// Clear screen when exiting
	clear();
	refresh();
}

// end of file: MenuClass.cpp
