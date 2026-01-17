#include "MenuTrade.h"
#include "../Ai/AiPlayer.h"
#include "../Ai/AiShopkeeper.h"
#include "MenuBuy.h"
#include "MenuSell.h"
#include "../Actor/Actor.h"
#include "../Systems/ShopKeeper.h"
#include "../Core/GameContext.h"
#include "../Systems/MessageSystem.h"
#include "../Systems/RenderingManager.h"

void Buy::on_selection(GameContext& ctx)
{
	// Use the shopkeeper's attached shop component
	if (shopkeeper.shop != nullptr)
	{
		ctx.menus->push_back(std::make_unique<MenuBuy>(ctx, *ctx.player, *shopkeeper.shop));
	}
	else
	{
		ctx.message_system->message(WHITE_BLACK_PAIR, "This shopkeeper has nothing to sell.", true);
	}
}

void Sell::on_selection(GameContext& ctx)
{
	ctx.menus->push_back(std::make_unique<MenuSell>(shopkeeper, player, ctx));
}

void Exit::on_selection(GameContext& ctx)
{
}

MenuTrade::MenuTrade(Creature& shopkeeper, Creature& player, GameContext& ctx) : shopkeeper(shopkeeper)
{
	menu_new(height_, width_, starty_, startx_, ctx);
	iMenuStates.push_back(std::make_unique<Buy>(shopkeeper));
	iMenuStates.push_back(std::make_unique<Sell>(shopkeeper, player));
	iMenuStates.push_back(std::make_unique<Exit>());
}

MenuTrade::~MenuTrade()
{
	menu_delete();
	
	// CRITICAL FIX: Reset shopkeeper trade state when menu closes
	if (shopkeeper.ai)
	{
		auto* shopAi = dynamic_cast<AiShopkeeper*>(shopkeeper.ai.get());
		if (shopAi)
		{
			shopAi->tradeMenuOpen = false;
		}
	}
}

void MenuTrade::menu_print_state(size_t state)
{
	if (currentState == state)
	{
		menu_highlight_on();
		menu_print(1, state + 1, menu_get_string(state)); // Start at row 1 after title
		menu_highlight_off();
	}
	else
	{
		menu_print(1, state + 1, menu_get_string(state)); // Start at row 1 after title
	}
}

void MenuTrade::draw_content()
{
	// Debug current state
	mvwprintw(menuWindow, 0, 0, "%d", currentState);
	
	// Draw menu options
	for (size_t i{ 0 }; i < menuStateStrings.size(); ++i)
	{
		menu_print_state(i);
	}
}

void MenuTrade::draw()
{
	menu_clear();
	box(menuWindow, 0, 0);
	// Title
	mvwprintw(menuWindow, 0, 1, "Trade");
	for (size_t i{ 0 }; i < menuStateStrings.size(); ++i)
	{
		menu_print_state(i);
	}
	menu_refresh();
}

void MenuTrade::on_key(int key, GameContext& ctx)
{
	switch (key)
	{

	case KEY_UP:
	case 'w':
	{
		currentState = (currentState + iMenuStates.size() - 1) % iMenuStates.size();
		break;
	}

	case KEY_DOWN:
	case 's':
	{
		currentState = (currentState + 1) % iMenuStates.size();
		break;
	}

	case 10:
	{ // if a selection is made
		menu_set_run_false();
		iMenuStates.at(currentState)->on_selection(ctx);
		break;
	}

	case 27:
	{
		menu_set_run_false();
		break;
	}

	default:
		break;
	}
}

void MenuTrade::menu(GameContext& ctx)
{
	// Save background before showing menu
	clear();
	ctx.rendering_manager->render(ctx);
	refresh();
	
	while (run)
	{
		draw();
		menu_key_listen();
		on_key(keyPress, ctx);
	}
	// Restore full game view
	clear();
	ctx.rendering_manager->render(ctx);
	refresh();
}