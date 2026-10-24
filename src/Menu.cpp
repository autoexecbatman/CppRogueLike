// file: Menu.cpp
#include <algorithm>
#include <cassert>
#include <iterator>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "GameContext.h"
#include "GameStateManager.h"
#include "Gui.h"
#include "ListMenu.h"
#include "Menu.h"
#include "MenuGender.h"
#include "MenuManager.h"
#include "MessageSystem.h"
#include "NotificationMenu.h"
#include "Renderer.h"
#include "RenderingManager.h"

#ifndef __EMSCRIPTEN__
#include "ItemEditor.h"
#include "MonsterEditor.h"
#include "RoomEditor.h"
#include "SpellEditor.h"
#endif

namespace
{
// Splits text into rows no wider than maxWidth pixels, each cut at a space where one fits.
// A word wider than maxWidth is kept whole on its own row rather than lost.
//
// Example, at the width of "The save file was refused and has been deleted." (513 pixels
// in a test's hidden window):
//   wrap_to_width("[json.exception.out_of_range.403] key 'map_width' not found", 513, renderer);
//   // -> { "[json.exception.out_of_range.403] key", "'map_width' not found" }
std::vector<std::string> wrap_to_width(
	std::string_view text,
	int maxWidth,
	const Renderer& renderer)
{
	std::vector<std::string> rows;
	while (!text.empty())
	{
		std::string row = renderer.fit_text_to_width(text, maxWidth);
		// Nothing fits when the first word is wider than the row, so that word takes a row alone.
		if (row.empty())
		{
			const size_t wordEnd = text.find(' ');
			row = std::string{ text.substr(
				0,
				wordEnd == std::string_view::npos ? text.size() :
													wordEnd + 1) };
		}
		text.remove_prefix(row.size());
		// The space a row was cut at belongs to neither row.
		while (!text.empty() && text.front() == ' ')
		{
			text.remove_prefix(1);
		}
		rows.push_back(std::move(row));
	}
	return rows;
}

// The notice's first line for a Load Game that left no game; a loaded game has no notice.
//
// Example:
//   unloaded_game_headline(LoadOutcomeKind::REFUSED_AND_KEPT);
//   // -> "The save file was refused and could not be deleted."
std::string_view unloaded_game_headline(LoadOutcomeKind kind)
{
	switch (kind)
	{
	case LoadOutcomeKind::LOADED:
	{
		assert(false && "unloaded_game_headline: a loaded game has no notice");
		return "";
	}

	case LoadOutcomeKind::NO_SAVE:
	{
		return "There is no save file to load.";
	}

	case LoadOutcomeKind::REFUSED:
	{
		return "The save file was refused and has been deleted.";
	}

	case LoadOutcomeKind::REFUSED_AND_KEPT:
	{
		return "The save file was refused and could not be deleted.";
	}
	}

	std::unreachable();
}

// Tells the player why Load Game left no game, then hands them a fresh main menu.
void report_unloaded_game(const LoadOutcome& outcome, GameContext& ctx)
{
	assert(ctx.renderer && "report_unloaded_game: called without a renderer");

	const bool refused = outcome.kind == LoadOutcomeKind::REFUSED ||
		outcome.kind == LoadOutcomeKind::REFUSED_AND_KEPT;
	const std::string headline{ unloaded_game_headline(outcome.kind) };
	std::vector<std::string> lines{ headline };
	if (refused)
	{
		lines.push_back("");
		std::ranges::move(
			wrap_to_width(
				outcome.reason,
				ctx.renderer->measure_text(headline),
				*ctx.renderer),
			std::back_inserter(lines));
	}
	auto notice = std::make_unique<NotificationMenu>(
		refused ? "SAVE CANNOT BE READ" : "NO SAVE FOUND",
		std::move(lines),
		ctx);
	// The stack is replaced, as the death screen does, so no menu of the abandoned load survives.
	const auto return_to_main_menu = [](GameContext& closeCtx)
	{
		closeCtx.menus->clear();
		closeCtx.menus->push_back(make_main_menu(true, closeCtx));
	};
	notice->set_on_close(return_to_main_menu);
	ctx.menus->push_back(std::move(notice));
}
} // namespace

std::unique_ptr<BaseMenu> make_main_menu(bool startup, GameContext& ctx)
{
	std::vector<MenuEntry> entries;

	auto newGameCommand = [](GameContext& ctx)
	{
		assert(
			ctx.menuManager &&
			"make_main_menu newGameCommand: called without a menuManager");

		ctx.menuManager->set_game_initialized(false);
		ctx.menus->push_back(make_gender_menu(ctx));
	};
	entries.push_back({ "New Game", 'n', newGameCommand });

	if (GameStateManager::save_file_exists())
	{
		auto loadGameCommand = [](GameContext& ctx)
		{
			assert(
				ctx.stateManager &&
				"make_main_menu loadGameCommand: called without a stateManager");

			const LoadOutcome outcome = ctx.stateManager->load_all(ctx);
			if (outcome.kind != LoadOutcomeKind::LOADED)
			{
				report_unloaded_game(outcome, ctx);
			}
		};
		entries.push_back({ "Load Game", 'l', loadGameCommand });
	}

	auto optionsCommand = [startup](GameContext& ctx)
	{
		ctx.menus->push_back(make_main_menu(startup, ctx));
	};
	entries.push_back({ "Options", 'o', optionsCommand });

#ifndef __EMSCRIPTEN__
	auto roomEditorCommand = [](GameContext& ctx)
	{
		assert(
			ctx.roomEditor &&
			"make_main_menu roomEditorCommand: called without a roomEditor");

		ctx.roomEditor->enter();
	};
	entries.push_back({ "Room Editor", 'e', roomEditorCommand });

	auto monsterEditorCommand = [](GameContext& ctx)
	{
		assert(
			ctx.monsterEditor &&
			"make_main_menu monsterEditorCommand: called without a monsterEditor");

		ctx.monsterEditor->enter(ctx);
	};
	entries.push_back({ "Monster Editor", 'm', monsterEditorCommand });

	auto spellEditorCommand = [](GameContext& ctx)
	{
		assert(
			ctx.spellEditor &&
			"make_main_menu spellEditorCommand: called without a spellEditor");

		ctx.spellEditor->enter(ctx);
	};
	entries.push_back({ "Spell Editor", 'p', spellEditorCommand });

	auto itemEditorCommand = [](GameContext& ctx)
	{
		assert(
			ctx.itemEditor &&
			"make_main_menu itemEditorCommand: called without an itemEditor");

		ctx.itemEditor->enter(ctx);
	};
	entries.push_back({ "Item Editor", 'i', itemEditorCommand });
#endif

	auto quitCommand = [](GameContext& ctx)
	{
		ctx.gameState->set_run(false);
		ctx.gameState->set_should_save(false);
		ctx.messageSystem->log("You quit without saving!");
	};
	entries.push_back({ "Quit", 'q', quitCommand });

	// ESC on the startup menu quits the game; in-game it just closes.
	std::function<void(GameContext&)> onEscape{};
	if (startup)
	{
		onEscape = quitCommand;
	}

	// In-game menu renders the world behind it each frame.
	std::function<void(GameContext&)> onFrame{};
	if (!startup)
	{
		onFrame = [](GameContext& ctx)
		{
			assert(
				ctx.menuManager &&
				"make_main_menu onFrame: called without a menuManager");
			assert(
				ctx.renderingManager &&
				"make_main_menu onFrame: called without a renderingManager");
			assert(ctx.gui && "make_main_menu onFrame: called without a gui");

			if (ctx.menuManager->is_game_initialized())
			{
				ctx.renderingManager->render(ctx);
				ctx.gui->gui_render(ctx);
			}
		};
	}

	return std::make_unique<ListMenu>(
		"MAIN MENU",
		std::move(entries),
		std::move(onEscape),
		std::move(onFrame),
		ctx);
}

// end of file: Menu.cpp
