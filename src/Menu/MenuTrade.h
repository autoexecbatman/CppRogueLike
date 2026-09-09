#pragma once

#include <vector>

#include "BaseMenu.h"
#include "MenuEntry.h"

class Creature;
struct GameContext;

// Opens the trade menu between a shopkeeper and the player, and suppresses input
// for the frame so the bump that opened it is not also read by the menu.
//
// Example:
//   open_trade(shopkeeper, player, ctx); // trade menu is now on top of ctx.menus
void open_trade(Creature& shopkeeper, Creature& player, GameContext& ctx);

class MenuTrade : public BaseMenu
{
private:
    int height{ 5 };
    int width{ 10 };
    int startY{ 0 };
    int startX{ 0 };

    size_t currentState{ 0 };
    std::vector<MenuEntry> entries{};

    void menu_print_state(size_t state);
    void draw_content() override;

public:
    MenuTrade(Creature& shopkeeper, Creature& player, GameContext& ctx);
    MenuTrade(const MenuTrade&) = delete;
    MenuTrade& operator=(const MenuTrade&) = delete;
    MenuTrade(MenuTrade&&) = delete;
    MenuTrade& operator=(MenuTrade&&) = delete;

    void draw();
    void on_key(GameContext& ctx) override;
    void menu(GameContext& ctx) override;
};
