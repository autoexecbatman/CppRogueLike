#pragma once

#include <deque>
#include <memory>

class BaseMenu;
struct GameContext;

class MenuManager
{
public:
	// Core menu management
	void handle_menus(std::deque<std::unique_ptr<BaseMenu>>& menus, GameContext& ctx);

	// Menu state queries
	bool has_active_menus(const std::deque<std::unique_ptr<BaseMenu>>& menus) const noexcept;

	// Game state accessors
	bool is_game_initialized() const noexcept { return gameWasInit; }
	void set_game_initialized(bool initialized) noexcept { gameWasInit = initialized; }

	bool should_take_input() const noexcept { return shouldTakeInput; }

	// Controls whether handle_input_phase polls for new keys this frame.
	// Set to false when pushing a menu that must consume the triggering keypress
	// before the game loop sees it (prevents the key that opened the menu from
	// also being processed as a game action). Reset to true by handle_input_phase
	// at the start of each frame.
	void set_should_take_input(bool takeInput) noexcept { shouldTakeInput = takeInput; }

private:
	bool gameWasInit{ false };
	bool shouldTakeInput{ true };

	void restore_game_display(GameContext& ctx);
};
