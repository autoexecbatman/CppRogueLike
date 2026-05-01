// file: MenuTrade.cpp
#include <memory>

#include "../Actor/Creature.h"
#include "../Colors/Colors.h"
#include "../Core/GameContext.h"
#include "../Renderer/Renderer.h"
#include "../Systems/MessageSystem.h"
#include "MenuBuy.h"
#include "MenuSell.h"
#include "MenuTrade.h"

MenuTrade::MenuTrade(Creature& shopkeeper, Creature& player, GameContext& ctx)
    : shopkeeper(shopkeeper)
{
    int vcols = ctx.renderer ? ctx.renderer->get_viewport_cols() : 60;
    int vrows = ctx.renderer ? ctx.renderer->get_viewport_rows() : 34;
    startY = (vrows - height) / 2;
    startX = (vcols - width) / 2;
    menu_new(width, height, startX, startY, ctx);

    auto buyCommand = [&shopkeeper](GameContext& ctx)
    {
        if (shopkeeper.shop != nullptr)
        {
            ctx.menus->push_back(std::make_unique<MenuBuy>(ctx, *ctx.player, *shopkeeper.shop));
        }
        else
        {
            ctx.messageSystem->message(WHITE_BLACK_PAIR, "This shopkeeper has nothing to sell.", true);
        }
    };
    entries.push_back({ "Buy", 0, buyCommand });

    auto sellCommand = [&shopkeeper, &player](GameContext& ctx)
    {
        ctx.menus->push_back(std::make_unique<MenuSell>(shopkeeper, player, ctx));
    };
    entries.push_back({ "Sell", 0, sellCommand });

    entries.push_back({ "Exit", 0, std::nullopt });
}

void MenuTrade::menu_print_state(size_t state)
{
    if (currentState == state)
    {
        menu_highlight_on();
        menu_print(1, static_cast<int>(state) + 1, entries[state].label);
        menu_highlight_off();
    }
    else
    {
        menu_print(1, static_cast<int>(state) + 1, entries[state].label);
    }
}

void MenuTrade::draw_content()
{
    for (size_t i{ 0 }; i < entries.size(); ++i)
    {
        menu_print_state(i);
    }
}

void MenuTrade::draw()
{
    menu_clear();
    menu_draw_box();
    menu_draw_title("TRADE", YELLOW_BLACK_PAIR);
    for (size_t i{ 0 }; i < entries.size(); ++i)
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
        currentState = (currentState + entries.size() - 1) % entries.size();
        break;
    }
    case 0x102: // DOWN
    case 's':
    {
        currentState = (currentState + 1) % entries.size();
        break;
    }
    case 10:
    {
        menu_set_run_false();
        if (entries[currentState].command)
        {
            (*entries[currentState].command)(ctx);
        }
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
    menu_key_listen();
    draw();
    on_key(keyPress, ctx);
}

// end of file: MenuTrade.cpp
