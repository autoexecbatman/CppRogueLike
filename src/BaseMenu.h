#pragma once

#include "Colors.h"

#include <optional>
#include <string>

#include "InputSystem.h"

class Renderer;
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
	GameKey lastKey{ GameKey::NONE };
	int lastChar{ 0 };
	bool isHighlighted{ false };

	// Paints one row at an already-resolved pixel column, with the highlight bar
	// when one is on. Both menu_print forms end here so a row is drawn one way.
	void menu_draw_row(int pixelX, int row, const std::string& text);

	// Which text row of this menu a pointer at this screen pixel sits on, or nothing when
	// it is outside the panel. rowCount is how many rows the menu is showing, so a pointer
	// below the last one answers nothing rather than a row that is not drawn.
	//
	// Example, a menu four tiles in and eight wide at a 64 pixel tile:
	//   menu_row_at(300, 200, 5);   // -> 1
	//   menu_row_at(900, 200, 5);   // -> nothing, right of the panel
	//   menu_row_at(300, 40, 5);    // -> nothing, on the frame above row 0
	[[nodiscard]] std::optional<size_t> menu_row_at(int pixelX, int pixelY, size_t rowCount) const;

	// True only on a frame where the pointer actually moved. A menu that re-seats its
	// cursor from a stationary pointer overwrites it every frame, and the arrow keys then
	// look broken while the pointer rests inside the panel.
	[[nodiscard]] bool menu_pointer_moved() const;

	// Whether this frame carries a click. Asked through InputSystem rather than Raylib:
	// menu_key_listen calls poll(), which consumes the press transition, so a raw
	// IsMouseButtonPressed in the same frame sees prev == curr and answers false.
	[[nodiscard]] bool menu_left_clicked() const;
	[[nodiscard]] bool menu_right_clicked() const;

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
	// Draws text at a tile column and a text row inside the menu. Row 0 sits just
	// below the top border. Rows run on a pixel pitch rather than the tile grid,
	// so a menu holds the same lines however far the map is zoomed.
	void menu_print(int x, int row, const std::string& text);
	// The same, horizontally centred inside the menu's borders by measured width.
	void menu_print_centered(int row, const std::string& text);
	// The shop screens' column header, on row 0 so it shares the pitch of the rows
	// beneath it rather than sitting on the tile grid.
	void menu_print_header();
	void menu_refresh();
	void menu_highlight_on() { isHighlighted = true; }
	void menu_highlight_off() { isHighlighted = false; }
	void menu_key_listen();
	void menu_set_run_true() { run = true; }
	void menu_set_run_false() { run = false; }
	void menu_draw_box();
	void menu_draw_title(std::string_view title, ColorPairId colorPair);

	virtual void menu(GameContext& ctx) = 0;
	virtual void draw_content() {}
	virtual void on_key(GameContext& ctx);
};
