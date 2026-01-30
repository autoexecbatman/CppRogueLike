#pragma once

#include <vector>
#include <string>

#include "BaseMenu.h"
#include "../Systems/SpellSystem.h"

struct GameContext;
class Player;

class MenuSpellCast : public BaseMenu
{
	std::vector<SpellId> availableSpells;
	std::vector<std::string> spellSources; // "" = memorized, "Ring", "Helm", etc.
	Player& player;
	GameContext& ctx;
	int selectedIndex{ 0 };

	void populate_spells();
	void draw_content() override;
	void handle_selection();

public:
	MenuSpellCast(GameContext& ctx, Player& player);
	~MenuSpellCast() override = default;
	MenuSpellCast(const MenuSpellCast&) = delete;
	MenuSpellCast& operator=(const MenuSpellCast&) = delete;
	MenuSpellCast(MenuSpellCast&&) = delete;
	MenuSpellCast& operator=(MenuSpellCast&&) = delete;

	void menu(GameContext& ctx) override;
};
