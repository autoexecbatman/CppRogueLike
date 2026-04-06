#pragma once

// ListMenu — a reusable vertical list menu.
// Replaces the duplicated MenuState-enum + two-map pattern that appeared
// identically in Menu, MenuClass, MenuRace, and MenuGender.
//
// Construct via the factory functions in Menu.h, MenuGender.h, MenuRace.h,
// MenuClass.h — do not instantiate directly from outside the Menu subsystem.
//
// Navigation: UP/DOWN (or w/s). Selection: ENTER or matching hotkey. ESC
// triggers the optional onEscape callback then closes the menu.

#include <functional>
#include <string>
#include <vector>

#include "BaseMenu.h"
#include "MenuEntry.h"

struct GameContext;

class ListMenu : public BaseMenu
{
    std::string title{};
    std::vector<MenuEntry> entries{};
    size_t cursorIndex{ 0 };
    std::function<void(GameContext&)> onEscape{};  // null = just close
    std::function<void(GameContext&)> onFrame{};   // null = no-op; called each frame before input

    void draw_entries();

public:
    ListMenu(
        std::string title,
        std::vector<MenuEntry> entries,
        std::function<void(GameContext&)> onEscape,
        std::function<void(GameContext&)> onFrame,
        GameContext& ctx);
    ~ListMenu() override;
    ListMenu(const ListMenu&) = delete;
    ListMenu& operator=(const ListMenu&) = delete;
    ListMenu(ListMenu&&) = delete;
    ListMenu& operator=(ListMenu&&) = delete;

    void draw();
    void on_key(int key, GameContext& ctx);
    void menu(GameContext& ctx) override;
};
