// file: main.cpp
#include <exception>
#include <format>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <vector>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif
#include <raylib.h>

#include "Game.h"
#include "Menu.h"
#include "Paths.h"
#include "StartupSteps.h"

namespace CppRL
{

#ifdef __EMSCRIPTEN__
struct LoopData
{
	Game* game{};
	int loopNum{};
};

void emscripten_loop(void* arg)
{
	auto* data = static_cast<LoopData*>(arg);
	if (WindowShouldClose() || !data->game->gameState.get_run())
	{
		emscripten_cancel_main_loop();
		return;
	}

	data->game->tick(data->loopNum);
}
#endif

} // namespace CppRL

int main()
{
	// Debug logging
	std::ofstream debugFile;
	try
	{
		debugFile.exceptions(std::ofstream::failbit | std::ofstream::badbit);
		debugFile.open(CppRL::Paths::resolve(CppRL::Paths::LOG));
		std::clog.rdbuf(debugFile.rdbuf());
	}
	catch (const std::exception& error)
	{
		const std::string warning =
			std::format("Warning: Could not open debug file: {}", error.what());
		std::cerr << warning << std::endl;
	}

	std::clog << "STARTUP: Opened debug log\n" << std::flush;

	std::clog << "STARTUP: Creating Game\n" << std::flush;
	// Game owns everything including Renderer and InputSystem
	auto game = std::make_unique<CppRL::Game>(std::random_device{}());

	const auto load_tile_config = [&game]()
	{
		game->tileConfig.load(CppRL::Paths::TILE_CONFIG);
	};
	const auto load_body_plans = [&game]()
	{
		game->bodyPlanRegistry.load(CppRL::Paths::BODY_PLANS);
	};
	const auto load_spells = [&game]()
	{
		game->spellRegistry.load(CppRL::Paths::SPELLS);
	};
	const auto load_monsters = [&game]()
	{
		game->monsterRegistry.load(CppRL::Paths::MONSTERS);
		game->monsterRegistry.refuse_unknown_spells(game->spellRegistry);
	};
	const auto load_items = [&game]()
	{
		game->itemRegistry.load(CppRL::Paths::ITEMS);
		game->itemRegistry.load_enhanced_rules(CppRL::Paths::ENHANCED_RULES);
	};
	const auto load_item_tiles = [&game]()
	{
		game->load_item_tiles();
	};
	const auto init_world = [&game]()
	{
		game->init_world();
	};
	// Opens the raylib window (fullscreen, auto-detect resolution); every step before it runs windowless.
	const auto init_renderer = [&game]()
	{
		game->renderer.init();
	};
	const auto load_tileset = [&game]()
	{
		game->renderer.load_dawnlike(CppRL::Paths::DAWNLIKE_DIR);
	};
	// The font size sets every HUD width. Jersey10 is an outline face, sized for legibility against the
	// 32 pixel row pitch rather than a bitmap multiple.
	const auto load_font = [&game]()
	{
		game->renderer.load_font(CppRL::Paths::DAWNLIKE_FONT, 30);
	};
	const auto load_palette = [&game]()
	{
		game->palette.load(CppRL::Paths::TILE_CONFIG);
	};
	const auto load_prefabs = [&game]()
	{
		game->prefabLibrary.load(CppRL::Paths::PREFABS);
	};

	const std::vector<CppRL::StartupStep> startupSteps{
		{ "Loading tile config", load_tile_config },
		{ "Loading body plans", load_body_plans },
		{ "Loading spells", load_spells },
		{ "Loading monsters", load_monsters },
		{ "Loading items", load_items },
		{ "Loading item tiles", load_item_tiles },
		{ "Initializing world", init_world },
		{ "Initializing renderer", init_renderer },
		{ "Loading Dawnlike tileset", load_tileset },
		{ "Loading font", load_font },
		{ "Loading palette", load_palette },
		{ "Loading prefabs", load_prefabs },
	};
	const std::optional<CppRL::StartupRefusal> refusal =
		CppRL::run_startup_steps(startupSteps, std::clog);
	if (refusal)
	{
		CppRL::report_startup_refusal(*refusal);
		return 1;
	}

	auto ctx = game->context();

	game->menus.push_back(CppRL::make_main_menu(true, ctx));

	int loopNum{ 0 };

#ifdef __EMSCRIPTEN__
	CppRL::LoopData loopData{ game.get(), loopNum };
	emscripten_set_main_loop_arg(CppRL::emscripten_loop, &loopData, 0, 1);
#else
	// Frame-based game loop
	while (!WindowShouldClose() && game->gameState.get_run())
	{
		game->tick(loopNum);
	}
#endif

	// Shutdown
	game->shutdown();
	game->renderer.shutdown();

	if (debugFile.is_open())
	{
		try
		{
			debugFile.close();
		}
		catch (const std::exception& error)
		{
			const std::string warning = std::format(
				"Warning: Exception closing debug file: {}",
				error.what());
			std::cerr << warning << std::endl;
		}
	}

	return 0;
}
