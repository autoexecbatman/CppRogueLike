#pragma once

#include <string>
#include <vector>

#include "../Systems/SpellSystem.h"
#include "BaseMenu.h"

struct GameContext;
class Player;

class MenuSpellCast : public BaseMenu
{
	std::vector<std::string> availableSpells;
	std::vector<std::string> spellSources; // "" = memorized, "Ring", "Helm", etc.
	Player& player;
	int selectedIndex{ 0 };
	bool initialized{ false };

	void populate_spells();
	void draw_content() override;
	void handle_selection(GameContext& ctx);

public:
	MenuSpellCast(Player& player);
	~MenuSpellCast() override = default;
	MenuSpellCast(const MenuSpellCast&) = delete;
	MenuSpellCast& operator=(const MenuSpellCast&) = delete;
	MenuSpellCast(MenuSpellCast&&) = delete;
	MenuSpellCast& operator=(MenuSpellCast&&) = delete;

	void menu(GameContext& ctx) override;
};
