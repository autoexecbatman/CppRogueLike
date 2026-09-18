// file: Gui.cpp
#include <algorithm>
#include <cassert>
#include <format>
#include <string>
#include <vector>

#include <raylib.h>

#include "Actor.h"
#include "Creature.h"
#include "Colors.h"
#include "GameContext.h"
#include "Persistent.h"
#include "Renderer.h"
#include "HungerSystem.h"
#include "MessageSystem.h"
#include "TileConfig.h"
#include "Gui.h"
#include "LogMessage.h"
#include "Player.h"

// Maximum log messages shown in the HUD
constexpr int LOG_MAX_MESSAGES = 5;

// UI_TEXT_ROW_PITCH, GUI_TEXT_TOP_INSET, GUI_TEXT_ROWS and gui_reserve_rows
// live in Renderer.h: the renderer has to reserve the HUD's height and the Gui
// has to fill it, so one of them holding the numbers privately puts the two out
// of step at any zoom but the one they were written for.

// Icons sit on the same pitch as the text beside them.
constexpr int GUI_ICON_SIZE = 32;
// Left margin inside the first panel, clear of the frame's rule at x=4..7. A
// half-tile was used here, which varies with zoom for a panel laid out in pixels,
// and at a 64-pixel tile it spent 32 pixels the bars needed.
constexpr int GUI_PANEL_LEFT_MARGIN = 12;
// Height of a bar, leaving a little air inside its row.
constexpr int GUI_BAR_HEIGHT = UI_TEXT_ROW_PITCH - 10;
// The frame sprites draw a 4-pixel rule inside their 64-pixel tile rather than at
// its edge: the outer left edge and both dividers put it 4 pixels in, and the
// right edge puts it 8 pixels short of the far side. Text has to clear the rule,
// not the tile, which is what these say. Measured off a render, not assumed.
constexpr int GUI_RULE_OFFSET = 4;
constexpr int GUI_RULE_WIDTH = 4;
// Clear air between a rule and the nearest glyph.
constexpr int GUI_TEXT_CLEARANCE = 12;

// The panel's background, matching the colour the frame sprites paint inside
// themselves. While the two differed, every frame tile showed as a 56-pixel band
// and any text near a divider sat on a visible box.
constexpr Color GUI_PANEL_BACKGROUND{ 20, 12, 28, 255 };

// ---------------------------------------------------------------------------
// HUD column layout, as fractions of the screen width:
//   Bar panel  : frame's left rule .. divider 1
//   Stat panel : divider 1 .. divider 2
//   Log panel  : divider 2 .. frame's right rule
// ---------------------------------------------------------------------------
// Both dividers are pixel positions proportional to the screen, not tile columns.
// A column-aligned divider can only move in whole tiles, which is coarser than the
// difference between a stats panel that fits its longest line and one that does
// not. Each of the first two panels gets what its widest content needs and the log
// gets the remainder, because it is the panel that runs out of room first.
//
// Sized against the font's advance, which is its load size in main.cpp, currently
// about 14.8 pixels a character. Re-measure these if that changes, and measure the
// advance rather than the ink: the ink of the last glyph stops short of its own
// advance, and fit_text_to_width counts the advance.
//   bars  - icon at 12, bar from 52, longest label is a hunger state at 8
//           characters, about 118px, so 182 needed
//   stats - every line is 12 characters or fewer, about 178px, so 210 needed
// Both panels hold only what the character sheet does not: the sheet carries the
// name, race, gold and all six ability scores in more detail than this ever did.
static int hud_divider1_x(int panelWidth)
{
	return panelWidth * 15 / 100;
}
static int hud_divider2_x(int panelWidth)
{
	return panelWidth * 32 / 100;
}

// Left edge of the text in the panel that starts at this divider.
static int hud_panel_text_left(int dividerX)
{
	return dividerX + GUI_RULE_OFFSET + GUI_RULE_WIDTH + GUI_TEXT_CLEARANCE;
}

// Right edge of the text in the panel that ends at this divider.
static int hud_panel_text_right(int dividerX)
{
	return dividerX - GUI_TEXT_CLEARANCE;
}

// Right edge of the last panel, which ends at the frame's own rule rather than at
// a divider. That rule sits near the far side of its tile, so the panel reaches
// most of a tile further right than the tile's left edge suggests.
static int hud_frame_text_right(int panelWidth)
{
	return panelWidth - GUI_RULE_OFFSET - GUI_RULE_WIDTH - GUI_TEXT_CLEARANCE;
}

// Top edge of the HUD panel. Measured up from the bottom of the screen rather than
// down the tile grid: whole tile rows rarely reach the bottom exactly - at a
// 96-pixel tile they stop 32 pixels short - and counting down the grid put the
// panel's contents that far above the panel's own floor.
static int hud_base_y(const Renderer& renderer)
{
	return renderer.get_screen_height() - renderer.get_gui_reserve_rows() * renderer.get_tile_size();
}

// Top edge of one HUD text row, counting from zero below the frame's top edge.
static int hud_text_row_y(int baseY, int tileSize, int row)
{
	return baseY + tileSize + GUI_TEXT_TOP_INSET + row * UI_TEXT_ROW_PITCH;
}

// How many text rows fit in the panel. The last row needs only the height of the
// font, not a whole pitch, which is why it is added back on.
static int hud_text_row_count(int tileSize, int fontSize)
{
	const int contentHeight =
		(gui_reserve_rows(tileSize, fontSize) - 1) * tileSize - GUI_TEXT_TOP_INSET - fontSize;
	return contentHeight / UI_TEXT_ROW_PITCH + 1;
}

// The rectangle log text is laid out in. Pixels across, rows down.
struct LogPanel
{
	int leftEdge{ 0 };
	int rightEdge{ 0 };
	int baseY{ 0 };
	int tileSize{ 0 };
	int rowCount{ 0 };
};

// Where the next piece of log text goes.
struct LogCursor
{
	int x{ 0 };
	int row{ 0 };
};

// Draws one coloured part of a message, wrapping onto further rows when it
// reaches the panel edge, and returns where the next part starts. Stops at the
// last row rather than drawing past the panel.
static LogCursor draw_log_part(
	const Renderer& renderer,
	const LogMessage& part,
	LogCursor cursor,
	const LogPanel& panel)
{
	std::string remainingText(part.logMessageText);
	while (!remainingText.empty() && cursor.row < panel.rowCount)
	{
		const std::string shown = renderer.fit_text_to_width(remainingText, panel.rightEdge - cursor.x);

		// Nothing readable fits in what is left of this row: start the next one.
		// Squeezing a two-letter fragment in first is what splits words.
		if (shown.empty())
		{
			cursor.x = panel.leftEdge;
			cursor.row++;
			continue;
		}

		renderer.draw_text(
			Vector2D{ cursor.x, hud_text_row_y(panel.baseY, panel.tileSize, cursor.row) },
			shown,
			part.logMessageColor);
		remainingText.erase(0, shown.size());

		// The part ended on this row, so the next part starts beside it.
		if (remainingText.empty())
		{
			cursor.x += renderer.measure_text(shown);
			break;
		}

		// The part was cut, so the rest of it continues on the next row, without
		// carrying the space the cut was made at.
		cursor.x = panel.leftEdge;
		cursor.row++;
		while (!remainingText.empty() && remainingText.front() == ' ')
		{
			remainingText.erase(0, 1);
		}
	}
	return cursor;
}

// ---------------------------------------------------------------------------

void Gui::add_display_message(const std::vector<LogMessage>& message)
{
	displayMessages.push_back(message);
}

void Gui::render_messages() noexcept {}

void Gui::gui_init() noexcept {}

void Gui::gui_shutdown() noexcept {}

void Gui::gui_update(GameContext& ctx)
{
	set_message(ctx.messageSystem->get_current_message());
	set_message_color(ctx.messageSystem->get_current_message_color());
}

// ---------------------------------------------------------------------------
// gui_render -- master HUD draw
// ---------------------------------------------------------------------------
void Gui::gui_render(const GameContext& ctx)
{
	assert(ctx.renderer && "Gui::gui_render called without a renderer");

	const int tileSize = ctx.renderer->get_tile_size();
	const int panelWidth = ctx.renderer->get_screen_width();
	const int reserveRows = ctx.renderer->get_gui_reserve_rows();
	const int baseY = hud_base_y(*ctx.renderer);
	const int ph = ctx.renderer->get_screen_height() - baseY;
	const int div1 = hud_divider1_x(panelWidth);
	const int div2 = hud_divider2_x(panelWidth);

	// The right edge sits against the screen rather than on the tile grid. Whole
	// tiles rarely fill the width exactly - at a 96-pixel tile they stop 32 pixels
	// short - and anchoring to the grid left that strip unpainted and took the
	// difference out of the panels.
	const int rightEdgeX = panelWidth - tileSize;

	// ---- Background -------------------------------------------------------
	DrawRectangle(0, baseY, panelWidth, ph, GUI_PANEL_BACKGROUND);

	// ---- Top border row (TL + T... + TR) ----------------------------------
	const auto& tileConfig = *ctx.tileConfig;
	ctx.renderer->draw_tile_screen(Vector2D{ 0, baseY }, tileConfig.get("GUI_FRAME_TL"));
	for (int col = tileSize; col < rightEdgeX; col += tileSize)
	{
		ctx.renderer->draw_tile_screen(Vector2D{ col, baseY }, tileConfig.get("GUI_FRAME_T"));
	}
	ctx.renderer->draw_tile_screen(Vector2D{ rightEdgeX, baseY }, tileConfig.get("GUI_FRAME_TR"));

	// ---- Left and right outer edges ---------------------------------------
	for (int row = 1; row < reserveRows; ++row)
	{
		ctx.renderer->draw_tile_screen(Vector2D{ 0, baseY + row * tileSize }, tileConfig.get("GUI_FRAME_L"));
		ctx.renderer->draw_tile_screen(Vector2D{ rightEdgeX, baseY + row * tileSize }, tileConfig.get("GUI_FRAME_R"));
	}

	// ---- Divider 1: bar panel | stat panel --------------------------------
	ctx.renderer->draw_tile_screen(Vector2D{ div1, baseY }, tileConfig.get("GUI_FRAME_T"));
	for (int row = 1; row < reserveRows; ++row)
	{
		ctx.renderer->draw_tile_screen(Vector2D{ div1, baseY + row * tileSize }, tileConfig.get("GUI_FRAME_L"));
	}

	// ---- Divider 2: stat panel | log panel --------------------------------
	ctx.renderer->draw_tile_screen(Vector2D{ div2, baseY }, tileConfig.get("GUI_FRAME_T"));
	for (int row = 1; row < reserveRows; ++row)
	{
		ctx.renderer->draw_tile_screen(Vector2D{ div2, baseY + row * tileSize }, tileConfig.get("GUI_FRAME_L"));
	}

	// ---- Panel content ----------------------------------------------------
	render_hp_bar(ctx);
	render_hunger_status(ctx);
	gui_print_stats(ctx);
	gui_print_log(ctx);
	render_player_status(ctx);
}

// ---------------------------------------------------------------------------
// Bar panel -- left section
// ---------------------------------------------------------------------------
void Gui::render_hp_bar(const GameContext& ctx)
{
	assert(ctx.renderer && "Gui::render_hp_bar called without a renderer");

	const int tileSize = ctx.renderer->get_tile_size();
	const int panelWidth = ctx.renderer->get_screen_width();
	const int baseY = hud_base_y(*ctx.renderer);
	const int div1 = hud_divider1_x(panelWidth);

	const int hp = ctx.player()->get_hp();
	const int maxHp = ctx.player()->get_max_hp();
	if (maxHp <= 0)
	{
		return;
	}

	const float ratio = std::clamp(
		static_cast<float>(hp) / static_cast<float>(maxHp), 0.0f, 1.0f);

	const int rowY = hud_text_row_y(baseY, tileSize, 0);

	// Heart icon in the first column, sized to the row rather than to a map tile.
	ctx.renderer->draw_tile_screen_sized(
		Vector2D{ GUI_PANEL_LEFT_MARGIN, rowY }, ctx.tileConfig->get("GUI_HEART_FULL"), GUI_ICON_SIZE);

	// Bar fills what is left of the panel, starting clear of the icon.
	const int barX = GUI_PANEL_LEFT_MARGIN + GUI_ICON_SIZE + 8;
	const int barW = hud_panel_text_right(div1) - barX;
	const int barH = GUI_BAR_HEIGHT;
	const int barY = rowY + (UI_TEXT_ROW_PITCH - barH) / 2;

	Color filled;
	if (ratio > 0.5f)
	{
		filled = ctx.renderer->get_color_pair(GREEN_BLACK_PAIR).fg;
	}
	else if (ratio > 0.25f)
	{
		filled = ctx.renderer->get_color_pair(YELLOW_BLACK_PAIR).fg;
	}
	else
	{
		filled = ctx.renderer->get_color_pair(RED_BLACK_PAIR).fg;
	}

	Color barEmpty = { 20, 20, 30, 255 };
	ctx.renderer->draw_bar(Vector2D{ barX, barY }, barW, barH, ratio, filled, barEmpty);

	// "HP 45/50" centered on the bar, and never left of it.
	// No "HP" prefix: the heart icon beside the bar already says what this counts,
	// and at three digits each the prefix was what set the bar panel's width.
	auto hpText = std::format("{}/{}", hp, maxHp);
	const int textW = ctx.renderer->measure_text(hpText);
	const int textX = barX + std::max(0, (barW - textW) / 2);
	ctx.renderer->draw_text(
		Vector2D{ textX, rowY + (UI_TEXT_ROW_PITCH - ctx.renderer->get_font_size()) / 2 },
		hpText,
		WHITE_BLACK_PAIR);
}

void Gui::render_hunger_status(const GameContext& ctx)
{
	assert(ctx.renderer && "Gui::render_hunger_status called without a renderer");

	const int tileSize = ctx.renderer->get_tile_size();
	const int panelWidth = ctx.renderer->get_screen_width();
	const int baseY = hud_base_y(*ctx.renderer);
	const int div1 = hud_divider1_x(panelWidth);

	if (ctx.hungerSystem->get_hunger_max() <= 0)
	{
		return;
	}

	const std::string hungerText = ctx.hungerSystem->get_hunger_state_string();
	const int hungerColor = ctx.hungerSystem->get_hunger_color();

	// Fullness, not hunger: this bar sits beside the health bar and has to fill
	// the same way, so a well fed creature shows a full one.
	const float ratio = ctx.hungerSystem->get_fullness_ratio();

	const int rowY = hud_text_row_y(baseY, tileSize, 1);

	// Food icon in the first column, on the same pitch as the heart above it.
	ctx.renderer->draw_tile_screen_sized(
		Vector2D{ GUI_PANEL_LEFT_MARGIN, rowY }, TileRef{ TileSheet::SHEET_FOOD, 0, 0 }, GUI_ICON_SIZE);

	// Bar fills what is left of the panel, starting clear of the icon.
	const int barX = GUI_PANEL_LEFT_MARGIN + GUI_ICON_SIZE + 8;
	const int barW = hud_panel_text_right(div1) - barX;
	const int barH = GUI_BAR_HEIGHT;
	const int barY = rowY + (UI_TEXT_ROW_PITCH - barH) / 2;

	Color filled = ctx.renderer->get_color_pair(YELLOW_BLACK_PAIR).fg;
	Color barEmpty = { 20, 20, 30, 255 };
	ctx.renderer->draw_bar(Vector2D{ barX, barY }, barW, barH, ratio, filled, barEmpty);

	// Hunger state centered on the bar, and never left of it.
	const int textW = ctx.renderer->measure_text(hungerText);
	const int textX = barX + std::max(0, (barW - textW) / 2);
	ctx.renderer->draw_text(
		Vector2D{ textX, rowY + (UI_TEXT_ROW_PITCH - ctx.renderer->get_font_size()) / 2 },
		hungerText,
		hungerColor);
}

// ---------------------------------------------------------------------------
// Stat panel -- middle section
// ---------------------------------------------------------------------------
void Gui::gui_print_stats(const GameContext& ctx) noexcept
{
	assert(ctx.renderer && "Gui::gui_print_stats called without a renderer");

	const int tileSize = ctx.renderer->get_tile_size();
	const int panelWidth = ctx.renderer->get_screen_width();
	const int baseY = hud_base_y(*ctx.renderer);
	const int statsX = hud_panel_text_left(hud_divider1_x(panelWidth));
	const int statsWidth = hud_panel_text_right(hud_divider2_x(panelWidth)) - statsX;

	if (ctx.player()->actorData.name.empty())
	{
		ctx.player()->actorData.name = "Player";
	}

	// The panel is as wide as its widest line, so every line here is twelve
	// characters or fewer and the width left over goes to the log. What is missing
	// is on the character sheet, which carries the name, race, gold and all six
	// ability scores with their derived modifiers.

	// Row 1: Name, which clips to the panel. Everything below it is sized to fit.
	ctx.renderer->draw_text(Vector2D{ statsX, hud_text_row_y(baseY, tileSize, 0) }, ctx.renderer->fit_text_to_width(ctx.player()->actorData.name, statsWidth), YELLOW_BLACK_PAIR);

	// Row 2: Class
	ctx.renderer->draw_text(Vector2D{ statsX, hud_text_row_y(baseY, tileSize, 1) }, ctx.renderer->fit_text_to_width(ctx.player_concrete().get_class_display_name(), statsWidth), YELLOW_BLACK_PAIR);

	// Row 3: Level and to-hit. T0 = THAC0 abbreviation.
	auto levelLine = std::format(
		"Lv.{}  T0:{}",
		ctx.player()->get_level(),
		ctx.player()->get_thaco());
	ctx.renderer->draw_text(Vector2D{ statsX, hud_text_row_y(baseY, tileSize, 2) }, ctx.renderer->fit_text_to_width(levelLine, statsWidth), WHITE_BLACK_PAIR);

	// Row 4: What stops a hit landing, and what it costs when one does.
	auto defenceLine = std::format(
		"AC:{}  DR:{}",
		ctx.player()->get_armor_class(),
		ctx.player()->get_dr());
	ctx.renderer->draw_text(Vector2D{ statsX, hud_text_row_y(baseY, tileSize, 3) }, ctx.renderer->fit_text_to_width(defenceLine, statsWidth), WHITE_BLACK_PAIR);

	// Row 5: Attack roll
	auto atkLine = std::format(
		"Atk: {}", ctx.player_concrete().get_equipped_weapon_damage_roll());
	ctx.renderer->draw_text(Vector2D{ statsX, hud_text_row_y(baseY, tileSize, 4) }, ctx.renderer->fit_text_to_width(atkLine, statsWidth), GREEN_BLACK_PAIR);

	// Row 6: Gold. The "gp" suffix went with the rest of the width.
	auto goldLine = std::format("Gold: {}", ctx.player_concrete().get_gold());
	ctx.renderer->draw_text(Vector2D{ statsX, hud_text_row_y(baseY, tileSize, 5) }, ctx.renderer->fit_text_to_width(goldLine, statsWidth), YELLOW_BLACK_PAIR);
}

// ---------------------------------------------------------------------------
// Log panel -- right section
// ---------------------------------------------------------------------------
void Gui::gui_print_log(const GameContext& ctx)
{
	assert(ctx.renderer && "Gui::gui_print_log called without a renderer");

	const int tileSize = ctx.renderer->get_tile_size();
	const int panelWidth = ctx.renderer->get_screen_width();
	const int baseY = hud_base_y(*ctx.renderer);
	const int logX = hud_panel_text_left(hud_divider2_x(panelWidth));

	const int messagesToShow = std::min(
		LOG_MAX_MESSAGES,
		static_cast<int>(ctx.messageSystem->get_stored_message_count()));

	const LogPanel panel{
		.leftEdge = logX,
		.rightEdge = hud_frame_text_right(panelWidth),
		.baseY = baseY,
		.tileSize = tileSize,
		.rowCount = hud_text_row_count(tileSize, ctx.renderer->get_font_size())
	};

	// Newest message first, each starting on a row of its own and wrapping as far
	// as the panel allows. Rows run out before messages do, which is why the count
	// above is a ceiling rather than the number drawn.
	LogCursor cursor{ .x = panel.leftEdge, .row = 0 };
	for (int messageIndex = 0; messageIndex < messagesToShow; ++messageIndex)
	{
		if (cursor.row >= panel.rowCount)
		{
			break;
		}

		const std::vector<LogMessage>& parts =
			ctx.messageSystem->get_attack_message_at(
				ctx.messageSystem->get_stored_message_count() - 1 - messageIndex);

		for (const LogMessage& part : parts)
		{
			cursor = draw_log_part(*ctx.renderer, part, cursor, panel);
		}

		// The next message begins on the row after this one ended.
		cursor.x = panel.leftEdge;
		cursor.row++;
	}
}

// ---------------------------------------------------------------------------
// Status effects (bar panel row 3)
// ---------------------------------------------------------------------------
void Gui::render_player_status(const GameContext& ctx)
{
	assert(ctx.renderer && "Gui::render_player_status called without a renderer");

	const int tileSize = ctx.renderer->get_tile_size();
	const int baseY = hud_base_y(*ctx.renderer);

	if (ctx.player()->has_state(ActorState::IS_CONFUSED))
	{
		ctx.renderer->draw_text(
			Vector2D{ GUI_PANEL_LEFT_MARGIN, hud_text_row_y(baseY, tileSize, 2) },
			"CONFUSED",
			RED_BLACK_PAIR);
	}
}

// The interface is redrawn from game state every frame, so it holds nothing
// of its own to persist.
void Gui::save(json& savedState)
{
}

void Gui::load(const json& savedState)
{
}

// end of file: Gui.cpp
