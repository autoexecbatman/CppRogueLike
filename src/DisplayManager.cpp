// file: Systems/DisplayManager.cpp

#include <format>
#include <memory>
#include <string>
#include <vector>

#include "Attacker.h"
#include "Player.h"
#include "Controls.h"
#include "GameContext.h"
#include "LevelManager.h"
#include "LevelUpSystem.h"
#include "NotificationMenu.h"
#include "BalanceViewer.h"
#include "CharacterSheetUI.h"
#include "LevelUpUI.h"
#include "MenuThiefSkills.h"
#include "DisplayManager.h"

void DisplayManager::display_help(GameContext& ctx) const
{
	std::vector<std::string> lines;
	lines.reserve(NAMED_COMMANDS.size() + CHARACTER_COMMANDS.size());

	// Keys that are not characters have to be named; everything else reads its key
	// off its own binding, so this screen cannot fall out of step with the game.
	for (const auto& named : NAMED_COMMANDS)
	{
		lines.push_back(std::format("{:<15} {}", named.description, named.keys));
	}

	for (const auto& command : CHARACTER_COMMANDS)
	{
		lines.push_back(std::format("{:<15} {}", command.description, command_key(command.control)));
	}

	ctx.menus->push_back(std::make_unique<NotificationMenu>("CONTROLS", std::move(lines), ctx));
}

void DisplayManager::display_levelup(Player& player, int xpLevel, GameContext& ctx) const
{
	// Apply all level up benefits through the new LevelUpSystem
	LevelUpSystem::apply_level_up_benefits(player, xpLevel, &ctx);

	// A thief's level is thirty discretionary points (PHB page 84), spent on a
	// screen of their own. Pushed under the summary so the player reads what the
	// level gave before deciding where it goes.
	push_thief_skill_allocation(player, xpLevel, ctx);

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
