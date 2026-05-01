#pragma once

#include <vector>

#include "BaseMenu.h"
#include "MenuEntry.h"

class Creature;
struct GameContext;

class MenuTrade : public BaseMenu
{
private:
    int height{ 5 };
    int width{ 10 };
    int startY{ 0 };
    int startX{ 0 };
    Creature& shopkeeper;

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
    void on_key(int key, GameContext& ctx);
    void menu(GameContext& ctx) override;
};
