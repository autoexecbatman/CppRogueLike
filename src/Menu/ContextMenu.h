#pragma once

#include <string>
#include <vector>

#include "BaseMenu.h"

struct GameContext;

class ContextMenu : public BaseMenu
{
public:
    ContextMenu(std::vector<std::string> options, int anchor_col, int anchor_row, GameContext& ctx);
    void menu(GameContext& ctx) override;
    int get_selected() const { return selected; }

private:
    std::vector<std::string> menuOptions;
    int selectedIndex{ 0 };
    int selected{ -1 }; // -1 = cancelled, 0..N = chosen option
    void draw_content() override;
};
