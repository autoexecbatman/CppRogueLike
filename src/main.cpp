// file: main.cpp
#include <exception>
#include <fstream>
#include <iostream>
#include <memory>

#ifdef EMSCRIPTEN
#include <emscripten/emscripten.h>
#endif
#include <raylib.h>

#include "Core/Paths.h"
#include "Factories/ItemCreator.h"
#include "Factories/MonsterCreator.h"
#include "Game.h"
#include "Menu/Menu.h"
#include "Systems/SpellSystem.h"

#ifdef EMSCRIPTEN
struct LoopData
{
	Game* game;
	int loopNum;
};

void emscripten_loop(void* arg)
{
	auto* data = static_cast<LoopData*>(arg);
	if (WindowShouldClose() || !data->game->game_state.get_run())
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

	// Load data before Game construction (MonsterFactory is built inside Map ctor).
	std::clog << "STARTUP: Loading MonsterCreator\n" << std::flush;
	MonsterCreator::load(Paths::MONSTERS);
	std::clog << "STARTUP: Loading SpellSystem\n" << std::flush;
	SpellSystem::load(Paths::SPELLS);
	std::clog << "STARTUP: Loading ItemCreator\n" << std::flush;
	ItemCreator::load(Paths::ITEMS);
	std::clog << "STARTUP: Loading enhanced rules\n" << std::flush;
	ItemCreator::load_enhanced_rules(Paths::ENHANCED_RULES);

	std::clog << "STARTUP: Creating Game\n" << std::flush;
	// Game owns everything including Renderer and InputSystem
	auto game = std::make_unique<Game>();
	std::clog << "STARTUP: Loading tile config\n" << std::flush;
	game->tile_config.load(Paths::TILE_CONFIG);
	std::clog << "STARTUP: Initializing world\n" << std::flush;
	game->init_world();

	std::clog << "STARTUP: Initializing renderer\n" << std::flush;
	// Initialize raylib window (fullscreen, auto-detect resolution)
	game->renderer.init();
	std::clog << "STARTUP: Loading Dawnlike tileset\n" << std::flush;
	game->renderer.load_dawnlike(Paths::DAWNLIKE_DIR);

	game->renderer.load_font(Paths::DAWNLIKE_FONT, 16);

	auto ctx = game->context();
	game->decor_editor.load_palette(Paths::TILE_CONFIG);
	game->prefab_library.load_tile_labels(Paths::TILE_CONFIG);
	game->prefab_library.load(Paths::PREFABS);

	game->menus.push_back(make_main_menu(true, ctx));

	int loopNum{ 0 };

#ifdef EMSCRIPTEN
	LoopData loopData{ game.get(), loopNum };
	emscripten_set_main_loop_arg(emscripten_loop, &loopData, 0, 1);
#else
	// Frame-based game loop
	while (!WindowShouldClose() && game->game_state.get_run())
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
