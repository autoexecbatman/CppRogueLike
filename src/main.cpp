// file: main.cpp
#include <exception>
#include <fstream>
#include <iostream>
#include <memory>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif
#include <raylib.h>

#include "Paths.h"
#include "ItemCreator.h"
#include "Game.h"
#include "Menu.h"

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

int main()
{
	// Debug logging
	std::ofstream debugFile;
	try
	{
		debugFile.exceptions(std::ofstream::failbit | std::ofstream::badbit);
		debugFile.open(Paths::resolve(Paths::LOG));
		std::clog.rdbuf(debugFile.rdbuf());
	}
	catch (const std::exception& e)
	{
		std::cerr << "Warning: Could not open debug file: " << e.what() << std::endl;
	}

	std::clog << "STARTUP: Opened debug log\n" << std::flush;

	// Load item data before Game construction (ItemFactory is built inside Map ctor).
	std::clog << "STARTUP: Loading ItemCreator\n" << std::flush;
	ItemCreator::load(Paths::ITEMS);
	std::clog << "STARTUP: Loading enhanced rules\n" << std::flush;
	ItemCreator::load_enhanced_rules(Paths::ENHANCED_RULES);

	std::clog << "STARTUP: Creating Game\n" << std::flush;
	// Game owns everything including Renderer and InputSystem
	auto game = std::make_unique<Game>();
	std::clog << "STARTUP: Loading tile config\n" << std::flush;
	game->tileConfig.load(Paths::TILE_CONFIG);
	std::clog << "STARTUP: Loading body plans\n" << std::flush;
	game->bodyPlanRegistry.load(Paths::BODY_PLANS);
	std::clog << "STARTUP: Loading spells\n" << std::flush;
	game->spellRegistry.load(Paths::SPELLS);
	std::clog << "STARTUP: Loading monsters\n" << std::flush;
	game->monsterRegistry.load(Paths::MONSTERS);
	std::clog << "STARTUP: Initializing world\n" << std::flush;
	game->init_world();

	std::clog << "STARTUP: Initializing renderer\n" << std::flush;
	// Initialize raylib window (fullscreen, auto-detect resolution)
	game->renderer.init();
	std::clog << "STARTUP: Loading Dawnlike tileset\n" << std::flush;
	game->renderer.load_dawnlike(Paths::DAWNLIKE_DIR);

	// This size is the text's width: the advance is whatever is asked for here, so
	// every layout width in the HUD is measured against it. 14 is a 1.75x bake of an
	// 8-pixel design, which antialiases the glyph edges slightly; the exact
	// multiples are 8, 16 and 24, and none of those is both readable and narrow.
	game->renderer.load_font(Paths::DAWNLIKE_FONT, 14);

	auto ctx = game->context();
	game->decorEditor.load_palette(Paths::TILE_CONFIG);
	game->prefabLibrary.load_tile_labels(Paths::TILE_CONFIG);
	game->prefabLibrary.load(Paths::PREFABS);

	game->menus.push_back(make_main_menu(true, ctx));

	int loopNum{ 0 };

#ifdef __EMSCRIPTEN__
	LoopData loopData{ game.get(), loopNum };
	emscripten_set_main_loop_arg(emscripten_loop, &loopData, 0, 1);
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
		catch (const std::exception& e)
		{
			std::cerr << "Warning: Exception closing debug file: " << e.what() << std::endl;
		}
	}

	return 0;
}
