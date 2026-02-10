// file: MenuClass.cpp
#include <algorithm>

#include "MenuClass.h"
#include "MenuName.h"
#include "../Random/RandomDice.h"
#include "../ActorTypes/Player.h"
#include "../Items/Items.h"
#include "../Items/Armor.h"
#include "../Actor/Pickable.h"
#include "../Actor/InventoryOperations.h"
#include "../ActorTypes/Healer.h"
#include "../Factories/ItemCreator.h"
#include "../Core/GameContext.h"
#include "../Systems/MessageSystem.h"

using namespace InventoryOperations; // For clean function calls

// TODO: This monster function should not exist.
void equip_fighter_starting_gear(GameContext& ctx)
{
	auto& player = *ctx.player;

	// AD&D 2e Fighter starting money: 5d4 × 10 gp (50-200 gp)
	int rollSum = 0;
	for(int i = 0; i < 5; i++)
	{
		rollSum += ctx.dice->d4();
	}
	int startingGold = rollSum * 10;
	player.set_gold(startingGold);
	
	// Generous starting equipment for solo play
	
	// PLATE MAIL (AC 3) - Add to inventory
	auto plate_mail = ItemCreator::create(ItemId::PLATE_MAIL, player.position);
	auto plate_mail_id = plate_mail->uniqueId; // Store ID before moving
	ctx.message_system->log("Created item with name: '" + plate_mail->actorData.name + "' and ID: " + std::to_string(plate_mail_id));
	ctx.message_system->log("Created plate mail with ID: " + std::to_string(plate_mail_id));
	auto plate_mail_result = add_item(player.inventory_data, std::move(plate_mail));
	if (plate_mail_result.has_value())
	{
		ctx.message_system->log("Plate mail added to inventory successfully");
		// Find plate mail by unique ID in inventory
		auto& inventory = player.inventory_data.items;
		auto item_it = std::find_if(inventory.begin(), inventory.end(),
			[plate_mail_id](const std::unique_ptr<Item>& item) 
			{
				return item && item->uniqueId == plate_mail_id;
			});
		if (item_it != inventory.end())
		{
			ctx.message_system->log("Found plate mail in inventory, attempting to equip");
			// Extract item from inventory for equipment
			auto extracted_item = std::move(*item_it);
			inventory.erase(item_it);
			if (extracted_item)
			{
				if (ctx.message_system->is_debug_mode()) ctx.message_system->log("DEBUG: Extracted item for equipment, proceeding to auto-detect slot");
				
				// Auto-detect correct slot based on proper item classification
				std::string itemName = extracted_item->actorData.name;
				if (ctx.message_system->is_debug_mode()) ctx.message_system->log("DEBUG: Checking item class: " + std::to_string(static_cast<int>(extracted_item->itemClass)));
				
				EquipmentSlot targetSlot;
				
				// Use proper item classification system instead of name parsing
				if (extracted_item->is_armor())
				{
					targetSlot = EquipmentSlot::BODY;
					if (ctx.message_system->is_debug_mode()) ctx.message_system->log("DEBUG: Detected as armor by classification, targeting BODY slot");
				}
				else if (extracted_item->is_shield())
				{
					targetSlot = EquipmentSlot::LEFT_HAND;
					if (ctx.message_system->is_debug_mode()) ctx.message_system->log("DEBUG: Detected as shield by classification, targeting LEFT_HAND slot");
				}
				else
				{
					ctx.message_system->log("ERROR: Unknown armor/shield type for " + itemName);
					add_item(player.inventory_data, std::move(extracted_item));
					return;
				}
				
				bool equipped = player.equip_item(std::move(extracted_item), targetSlot, ctx);
				if (equipped)
				{
					ctx.message_system->log("SUCCESS: " + itemName + " equipped to correct slot");
				}
				else
				{
					ctx.message_system->log("ERROR: Failed to equip " + itemName + " - item validation failed or disappeared");
				}
			}
		}
	}
	else
	{
		ctx.message_system->log("ERROR: Failed to add plate mail to inventory");
	}

	// LONG SWORD (1d8) - Add to inventory
	auto long_sword = ItemCreator::create(ItemId::LONG_SWORD, player.position);
	auto long_sword_id = long_sword->uniqueId; // Store ID before moving
	ctx.message_system->log("Created item with name: '" + long_sword->actorData.name + "' and ID: " + std::to_string(long_sword_id));
	ctx.message_system->log("Created long sword with ID: " + std::to_string(long_sword_id));
	auto sword_result = add_item(player.inventory_data, std::move(long_sword));
	if (sword_result.has_value())
	{
		ctx.message_system->log("Long sword added to inventory successfully");
		// Find long sword by unique ID in inventory
		auto& inventory2 = player.inventory_data.items;
		auto sword_it = std::find_if(inventory2.begin(), inventory2.end(),
			[long_sword_id](const std::unique_ptr<Item>& item) 
			{
				return item && item->uniqueId == long_sword_id;
			});
		if (sword_it != inventory2.end())
		{
			ctx.message_system->log("Found long sword in inventory, attempting to equip");
			// Extract item from inventory for equipment
			auto extracted_item = std::move(*sword_it);
			inventory2.erase(sword_it);
			if (extracted_item)
			{
				// Store item name for logging before potential move
				std::string itemName = extracted_item->actorData.name;
				bool equipped = player.equip_item(std::move(extracted_item), EquipmentSlot::RIGHT_HAND, ctx);
				if (equipped)
				{
					ctx.message_system->log("SUCCESS: " + itemName + " equipped to RIGHT_HAND slot");
					ctx.message_system->log("Current weapon damage: " + player.attacker->get_attack_damage(player).displayRoll);
				}
				else
				{
					ctx.message_system->log("ERROR: Failed to equip " + itemName + " - item validation failed or disappeared");
				}
			}
		}
		else
		{
			ctx.message_system->log("ERROR: Could not find long sword in inventory after adding");
		}
	}
	else
	{
		ctx.message_system->log("ERROR: Failed to add long sword to inventory");
	}
	
	// Sync ranged state and update combat stats from equipped weapons
	player.sync_ranged_state(ctx);
	
	// Update AC based on equipped armor (proper way)
	player.destructible->update_armor_class(player, ctx);

	// HEALING POTIONS (3x)
	for(int i = 0; i < 3; i++)
	{
		add_item(player.inventory_data, ItemCreator::create(ItemId::HEALTH_POTION, player.position));
	}
	
	// Log the rolled starting gold amount
	ctx.message_system->log("Fighter starting gold: " + std::to_string(startingGold) + " gp (5d4×10)");
	ctx.message_system->message(WHITE_BLACK_PAIR, "You have " + std::to_string(startingGold) + " gold pieces.", true);
}

void Fighter::on_selection(GameContext& ctx)
{
	ctx.player->playerClass = "Fighter";
	ctx.player->playerClassState = Player::PlayerClassState::FIGHTER;
	// Starting gear equipped in Game::update() STARTUP phase after init()
}

void Rogue::on_selection(GameContext& ctx)
{
	ctx.player->playerClass = "Rogue";
	ctx.player->playerClassState = Player::PlayerClassState::ROGUE;
}

void Cleric::on_selection(GameContext& ctx)
{
	ctx.player->playerClass = "Cleric";
	ctx.player->playerClassState = Player::PlayerClassState::CLERIC;
}

void Wizard::on_selection(GameContext& ctx)
{
	ctx.player->playerClass = "Wizard";
	ctx.player->playerClassState = Player::PlayerClassState::WIZARD;
}

void ClassRandom::on_selection(GameContext& ctx)
{
	switch (ctx.dice->d4())
	{
	case 1:
		ctx.player->playerClass = "Fighter";
		ctx.player->playerClassState = Player::PlayerClassState::FIGHTER;
		break;
	case 2:
		ctx.player->playerClass = "Rogue";
		ctx.player->playerClassState = Player::PlayerClassState::ROGUE;
		break;
	case 3:
		ctx.player->playerClass = "Wizard";
		ctx.player->playerClassState = Player::PlayerClassState::WIZARD;
		break;
	case 4:
		ctx.player->playerClass = "Cleric";
		ctx.player->playerClassState = Player::PlayerClassState::CLERIC;
		break;
	default:break;
	}
}

void ClassBack::on_selection(GameContext& ctx)
{
	ctx.menus->back()->back = true;
}

MenuClass::MenuClass(GameContext& ctx)
{
	menu_new(menu_height, menu_width, menu_starty, menu_startx, ctx);
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

void MenuClass::on_key(int key, GameContext& ctx)
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
		iMenuStates.at(currentState)->on_selection(ctx);
		if (currentState != MenuState::BACK)
		{
			MenuName menuName;
			menuName.menu_name(ctx);
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

void MenuClass::menu(GameContext& ctx)
{
	while (run)
	{
		draw();
		menu_key_listen();
		on_key(keyPress, ctx);
	}
	// Clear screen when exiting
	clear();
	refresh();
}

// end of file: MenuClass.cpp
