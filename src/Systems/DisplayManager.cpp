// file: Systems/DisplayManager.cpp

#include <memory>
#include <string>
#include <vector>

#include "../Actor/Attacker.h"
#include "../ActorTypes/Player.h"
#include "../Core/GameContext.h"
#include "../Systems/LevelManager.h"
#include "../Systems/LevelUpSystem.h"
#include "../Menu/NotificationMenu.h"
#include "../Tools/BalanceViewer.h"
#include "../UI/CharacterSheetUI.h"
#include "../UI/LevelUpUI.h"
#include "DisplayManager.h"

void DisplayManager::display_help(GameContext& ctx) const
{
	ctx.menus->push_back(std::make_unique<NotificationMenu>(
		"CONTROLS",
		std::vector<std::string>{
			"Movement  : numpad / arrow keys",
			"Wait      : numpad 5 / period (.)",
			"Pick up   : comma (,)",
			"Inventory : i",
			"Drop      : d",
			"Character : @",
			"Spells    : s",
			"Target    : t  (ranged attack)",
			"Rest      : r",
			"Help      : ?",
			"Quit      : q",
		},
		ctx));
}

void DisplayManager::display_levelup(Player& player, int xpLevel, GameContext& ctx) const
{
	// Apply all level up benefits through the new LevelUpSystem
	LevelUpSystem::apply_level_up_benefits(player, xpLevel, &ctx);

	// Display the level up screen using the dedicated UI class
	ctx.menus->push_back(std::make_unique<LevelUpUI>(player, xpLevel));
}

void DisplayManager::display_character_sheet(const Player& player, GameContext& ctx) const noexcept
{
	ctx.menus->push_back(std::make_unique<CharacterSheetUI>(player));
}

void DisplayManager::display_balance_viewer(GameContext& ctx) const
{
	const int level = ctx.levelManager->get_dungeon_level();
	ctx.menus->push_back(std::make_unique<BalanceViewer>(level, ctx));
}

// end of file: Systems/DisplayManager.cpp
