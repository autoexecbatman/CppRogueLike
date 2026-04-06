#pragma once

#include <string>
#include <vector>

#include "../Actor/Actor.h"
#include "BaseMenu.h"
#include "MenuEntry.h"

struct GameContext;

class MenuTrade : public BaseMenu
{
    int height_{ 5 };
    int width_{ 10 };
    int starty_{ 0 };
    int startx_{ 0 };
    Creature& shopkeeper;

    size_t currentState{ 0 };
    std::vector<MenuEntry> entries{};

    void menu_print_state(size_t state);
    void draw_content() override;

public:
    MenuTrade(Creature& shopkeeper, Creature& player, GameContext& ctx);
    ~MenuTrade();
    MenuTrade(const MenuTrade&) = delete;
    MenuTrade& operator=(const MenuTrade&) = delete;
    MenuTrade(MenuTrade&&) = delete;
    MenuTrade& operator=(MenuTrade&&) = delete;

    void draw();
    void on_key(int key, GameContext& ctx);
    void menu(GameContext& ctx) override;
};
