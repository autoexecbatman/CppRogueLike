#pragma once

#include <string>
#include <vector>

#include "SpellSystem.h"
#include "BaseMenu.h"

struct GameContext;
class Player;

class MenuSpellCast : public BaseMenu
{
    std::vector<std::string> availableSpells;
    std::vector<std::string> spellSources; // "" = memorized, "Ring", "Helm", etc.
    Player& player;
    int selectedIndex{ 0 };

    void populate_spells();
    void draw();
    void handle_selection(GameContext& ctx);
    void on_key(GameContext& ctx) override;

    // One spell's row, as drawn. The constructor measures these to size the box, so
    // the text that sets the width is the text that goes in it.
    [[nodiscard]] std::string spell_line(size_t index) const;

public:
    MenuSpellCast(Player& player, GameContext& ctx);
    MenuSpellCast(const MenuSpellCast&) = delete;
    MenuSpellCast& operator=(const MenuSpellCast&) = delete;
    MenuSpellCast(MenuSpellCast&&) = delete;
    MenuSpellCast& operator=(MenuSpellCast&&) = delete;

    void menu(GameContext& ctx) override;
};
