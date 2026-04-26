#pragma once

#include <string>

class Renderer;
class InputSystem;
class TileConfig;
struct GameContext;

class BaseMenu
{
protected:
	Renderer* renderer{ nullptr };
	InputSystem* inputSystem{ nullptr };
	const TileConfig* tileConfig{ nullptr };
	size_t menuWidth{ 0 };
	size_t menuHeight{ 0 };
	size_t menuStartX{ 0 };
	size_t menuStartY{ 0 };
	int keyPress{ 0 };
	bool isHighlighted{ false };

public:
	bool run{ true };
	bool back{ false };

	BaseMenu() = default;
	virtual ~BaseMenu() = default;
	BaseMenu(const BaseMenu&) = delete;
	BaseMenu& operator=(const BaseMenu&) = delete;
	BaseMenu(BaseMenu&&) = delete;
	BaseMenu& operator=(BaseMenu&&) = delete;

	void menu_new(size_t width, size_t height, size_t startX, size_t startY, GameContext& ctx);
	void menu_clear();
	void menu_print(int x, int y, const std::string& text);
	void menu_refresh();
	void menu_highlight_on() { isHighlighted = true; }
	void menu_highlight_off() { isHighlighted = false; }
	void menu_key_listen();
	void menu_set_run_true() { run = true; }
	void menu_set_run_false() { run = false; }
	void menu_draw_box();
	void menu_draw_title(std::string_view title, int colorPair);

	virtual void menu(GameContext& ctx) = 0;
	virtual void draw_content() {}
};
