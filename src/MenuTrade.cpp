// file: MenuTrade.cpp
#include <algorithm>
#include <cassert>
#include <memory>

#include "Creature.h"
#include "Colors.h"
#include "GameContext.h"
#include "Renderer.h"
#include "MessageSystem.h"
#include "MenuBuy.h"
#include "MenuSell.h"
#include "MenuManager.h"
#include "MenuTrade.h"

MenuTrade::MenuTrade(Creature& shopkeeper, Creature& player, GameContext& ctx)
{
    assert(ctx.renderer && "MenuTrade: renderer required before construction");

    auto buyCommand = [&shopkeeper](GameContext& ctx)
    {
        if (shopkeeper.shop != nullptr)
        {
            ctx.menus->push_back(std::make_unique<MenuBuy>(ctx, *ctx.player(), *shopkeeper.shop));
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

    const int tileSize = ctx.renderer->get_tile_size();
    int widestText = ctx.renderer->measure_text("TRADE");
    for (const auto& entry : entries)
    {
        widestText = std::max(widestText, ctx.renderer->measure_text(entry.label));
    }

    width = panel_tiles_for_text_width(widestText, tileSize);
    height = panel_tiles_for_text_rows(
        static_cast<int>(entries.size()), tileSize, ctx.renderer->get_font_size());
    startY = (ctx.renderer->get_viewport_rows() - height) / 2;
    startX = (ctx.renderer->get_viewport_cols() - width) / 2;
    menu_new(width, height, startX, startY, ctx);
}

void MenuTrade::menu_print_state(size_t state)
{
    if (currentState == state)
    {
        menu_highlight_on();
        menu_print(1, static_cast<int>(state), entries[state].label);
        menu_highlight_off();
    }
    else
    {
        menu_print(1, static_cast<int>(state), entries[state].label);
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

void MenuTrade::on_key(GameContext& ctx)
{
    if (lastKey == GameKey::UP || lastKey == GameKey::W)
    {
        currentState = (currentState + entries.size() - 1) % entries.size();
    }
    else if (lastKey == GameKey::DOWN || lastKey == GameKey::S)
    {
        currentState = (currentState + 1) % entries.size();
    }
    else if (lastKey == GameKey::ENTER)
    {
        menu_set_run_false();
        if (entries[currentState].command)
        {
            (*entries[currentState].command)(ctx);
        }
    }
    else if (lastKey == GameKey::ESCAPE)
    {
        menu_set_run_false();
    }
}

void MenuTrade::menu(GameContext& ctx)
{
    menu_key_listen();
    draw();
    on_key(ctx);
}

// end of file: MenuTrade.cpp

void open_trade(Creature& shopkeeper, Creature& player, GameContext& ctx)
{
	ctx.menus->push_back(std::make_unique<MenuTrade>(shopkeeper, player, ctx));
	ctx.menuManager->set_should_take_input(false);
}
