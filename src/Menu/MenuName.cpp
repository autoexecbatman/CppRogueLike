// file: MenuName.cpp
#include <string>
#include <cstring>

#include "MenuName.h"
#include "../Core/GameContext.h"
#include "../ActorTypes/Player.h"
#include "../Systems/MenuManager.h"
#include "../Systems/RenderingManager.h"

void MenuName::menu_name_new() noexcept { /* TODO: Reimplement with Panel+Renderer */ }

void MenuName::menu_name_clear() noexcept { /* TODO: Reimplement with Panel+Renderer */ }

void MenuName::menu_name_print(int x, int y, const std::string& text) noexcept { /* TODO: Reimplement with Panel+Renderer */ }

void MenuName::menu_name_refresh() noexcept { /* TODO: Reimplement with Panel+Renderer */ }

void MenuName::menu_name_delete() noexcept { /* TODO: Reimplement with Panel+Renderer */ }

std::string MenuName::menu_name_input()
{
	// TODO: Reimplement text input with InputSystem::get_char_input()
	// For now return default name
	return "Player";
}

void MenuName::menu_name_store() { playerName = name; }

void MenuName::menu_name_assign(GameContext& ctx) { ctx.player->actorData.name = playerName; }

void MenuName::menu_name(GameContext& ctx)
{
	menu_name_new();
	menu_name_clear();

	menu_name_print(0, 0, "Enter your name: ");

	std::string inputName = menu_name_input();
	std::strncpy(name, inputName.c_str(), sizeof(name) - 1);
	name[sizeof(name) - 1] = '\0';

	menu_name_store();
	menu_name_assign(ctx);

	menu_name_refresh();
	menu_name_delete();

	if (ctx.menu_manager->is_game_initialized())
	{
		ctx.rendering_manager->restore_game_display();
	}
}

// end of file: MenuName.cpp
