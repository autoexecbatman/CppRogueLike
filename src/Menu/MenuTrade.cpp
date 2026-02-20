#include "MenuTrade.h"
#include "../Ai/AiPlayer.h"
#include "../Ai/AiShopkeeper.h"
#include "MenuBuy.h"
#include "MenuSell.h"
#include "../Actor/Actor.h"
#include "../Systems/ShopKeeper.h"
#include "../Core/GameContext.h"
#include "../Renderer/Renderer.h"
#include "../Colors/Colors.h"
#include "../Systems/MessageSystem.h"
#include "../Systems/RenderingManager.h"

void Buy::on_selection(GameContext& ctx)
{
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
	int vcols = ctx.renderer ? ctx.renderer->get_viewport_cols() : 60;
	int vrows = ctx.renderer ? ctx.renderer->get_viewport_rows() : 34;
	starty_ = (vrows - height_) / 2;
	startx_ = (vcols - width_) / 2;
	menu_new(height_, width_, starty_, startx_, ctx);
	iMenuStates.push_back(std::make_unique<Buy>(shopkeeper));
	iMenuStates.push_back(std::make_unique<Sell>(shopkeeper, player));
	iMenuStates.push_back(std::make_unique<Exit>());
}

MenuTrade::~MenuTrade()
{
	menu_delete();

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
		menu_print(1, static_cast<int>(state) + 1, menu_get_string(state));
		menu_highlight_off();
	}
	else
	{
		menu_print(1, static_cast<int>(state) + 1, menu_get_string(state));
	}
}

void MenuTrade::draw_content()
{
	// TODO: Reimplement with Panel+Renderer
	for (size_t i{ 0 }; i < menuStateStrings.size(); ++i)
	{
		menu_print_state(i);
	}
}

void MenuTrade::draw()
{
	menu_clear();
	menu_draw_box();
	menu_draw_title("TRADE", YELLOW_BLACK_PAIR);
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
	case 0x103: // UP
	case 'w':
	{
		currentState = (currentState + iMenuStates.size() - 1) % iMenuStates.size();
		break;
	}

	case 0x102: // DOWN
	case 's':
	{
		currentState = (currentState + 1) % iMenuStates.size();
		break;
	}

	case 10:
	{
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
	ctx.rendering_manager->render(ctx);

	while (run && !WindowShouldClose())
	{
		draw();
		menu_key_listen();
		on_key(keyPress, ctx);
	}
	ctx.rendering_manager->render(ctx);
}
