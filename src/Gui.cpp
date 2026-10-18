// file: Gui.cpp
#include <algorithm>
#include <cassert>
#include <format>
#include <string>
#include <vector>

#include <raylib.h>

#include "Actor.h"
#include "Colors.h"
#include "Creature.h"
#include "GameContext.h"
#include "Gui.h"
#include "HungerSystem.h"
#include "LogMessage.h"
#include "MessageSystem.h"
#include "Persistent.h"
#include "Player.h"
#include "Renderer.h"
#include "TileConfig.h"

// Maximum log messages shown in the HUD
constexpr int LOG_MAX_MESSAGES = 5;

// UI_TEXT_ROW_PITCH, GUI_TEXT_TOP_INSET, GUI_TEXT_ROWS and gui_reserve_rows
// live in Renderer.h: the renderer has to reserve the HUD's height and the Gui
// has to fill it, so one of them holding the numbers privately puts the two out
// of step at any zoom but the one they were written for.

// How far left a divider's cell is aimed so its rail lands where the old one did.
// GUI_FRAME_L puts its rail against the left of its cell; GUI_FRAME_V centres its rail,
// so without this the divider would step right by half the difference.
constexpr int DIVIDER_CELL_OFFSET = 17;

// An icon shares its row with a bar, so the two are drawn the same height.
constexpr int GUI_ICON_SIZE = UI_TEXT_ROW_PITCH;
// A bar is a whole text row tall, which is the height its art is drawn at. The air
// inside the row is in the art, as clear rows above and below its rails.
constexpr int GUI_BAR_HEIGHT = UI_TEXT_ROW_PITCH;
// Clear air between a rule and the nearest glyph.
constexpr int GUI_TEXT_CLEARANCE = 12;

// The dark of a trough the fill has not reached.
constexpr Color GUI_BAR_CHANNEL{ 20, 20, 30, 255 };
// A white tint leaves a sprite its own colours.
constexpr Color GUI_UNTINTED{ 255, 255, 255, 255 };
// Laid under a reading that has to stay legible over any fill colour.
constexpr Color GUI_TEXT_SHADOW{ 0, 0, 0, 200 };

// The whole HUD strip is painted with this before the frame goes over it. Every
// frame sprite is transparent outside the rule it paints - the divider's cell
// covers thirty of its sixty-four columns - so nothing in the art supplies a panel
// colour and the map would otherwise show through.
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
// Sized against the font's advance, which is the load size in main.cpp and nothing
// in the font file, so a change there is what makes these fractions wrong. Measure
// the advance rather than the ink: the ink of the last glyph stops short of its own
// advance, and fit_text_to_width counts the advance.
//   bars  - an icon at the left margin, then a bar running to the divider with its
//           own reading centred on it, which is the part that overruns if it narrows
//   stats - one stat a row, its label at the left edge and its value flush right
// Both panels hold only what the character sheet does not: the sheet carries the
// name, race, gold and all six ability scores in more detail than this ever did.
static int hud_divider1_x(int panelWidth)
{
	return panelWidth * 15 / 100;
}

// The stat panel is as wide as its longest label-and-value line, which is Gold carrying
// a six-figure purse: that line and the clearance either side of it are what this
// fraction has to cover, and it is measured against a render rather than assumed. Every
// pixel beyond it belongs to the log, which is the panel that runs out of room first.
static int hud_divider2_x(int panelWidth)
{
	return panelWidth * 33 / 100;
}

// Left margin inside the first panel, clear of the frame's left rule.
static int hud_panel_left_margin(int tileSize)
{
	return gui_frame_rule_width(tileSize) + GUI_TEXT_CLEARANCE;
}

// Left edge of the text in the panel that starts at this divider. A divider's rule
// paints to the right of the line it is aimed at, so the text clears the whole of it.
static int hud_panel_text_left(int dividerX, int tileSize)
{
	return dividerX + gui_frame_rule_width(tileSize) + GUI_TEXT_CLEARANCE;
}

// Right edge of the text in the panel that ends at this divider.
static int hud_panel_text_right(int dividerX)
{
	return dividerX - GUI_TEXT_CLEARANCE;
}

// Right edge of the last panel, which ends at the frame's own rule rather than at
// a divider. That rule sits near the far side of its tile, so the panel reaches
// most of a tile further right than the tile's left edge suggests.
static int hud_frame_text_right(int panelWidth, int tileSize)
{
	return panelWidth - gui_frame_rule_width(tileSize) - GUI_TEXT_CLEARANCE;
}

// Top edge of the HUD panel. Measured up from the bottom of the screen rather than
// down the tile grid: whole tile rows rarely reach the bottom exactly - at a
// 96-pixel tile they stop 32 pixels short - and counting down the grid put the
// panel's contents that far above the panel's own floor.
static int hud_base_y(const Renderer& renderer)
{
	return renderer.get_screen_height() - renderer.get_gui_reserve_rows() * renderer.get_tile_size();
}

// Top edge of one HUD text row, counting from zero below the frame's rule. The rule is
// what the text has to clear; the rest of the edge tile is transparent.
static int hud_text_row_y(int baseY, int tileSize, int row)
{
	return baseY + gui_frame_top_rule(tileSize) + GUI_TEXT_TOP_INSET + row * UI_TEXT_ROW_PITCH;
}

// How many text rows fit in the panel. The last row needs only the height of the
// font, not a whole pitch, which is why it is added back on.
static int hud_text_row_count(int tileSize, int fontSize)
{
	const int contentHeight = gui_reserve_rows(tileSize, fontSize) * tileSize - gui_frame_top_rule(tileSize) - gui_frame_bottom_rule(tileSize) - GUI_TEXT_TOP_INSET - fontSize;
	return contentHeight / UI_TEXT_ROW_PITCH + 1;
}

// A bar in its trough: the dark channel, the fill over the share the bar has, the rim
// over both, then an end plate at each end. The rim's rails are opaque and cover the
// fill's top and bottom edges, so how deep they are stays the art's business and no
// number here describes them. The rim is stretched to the bar's width and the plates
// are not, which is why the ornament is on the plates.
static void draw_hud_bar(
	const GameContext& ctx,
	Vector2D pos,
	int width,
	float ratio,
	Color fillTint)
{
	assert(ctx.tileConfig && "draw_hud_bar: called without a tileConfig");
	assert(width > 2 * GUI_BAR_PLATE_WIDTH && "a bar narrower than its two end plates has no trough");

	const TileConfig& tileConfig = *ctx.tileConfig;

	// The trough runs between the opaque end plates, so the last few percent stays in sight.
	const Vector2D troughPos{ pos.x + GUI_BAR_PLATE_WIDTH, pos.y };
	const int troughWidth = width - 2 * GUI_BAR_PLATE_WIDTH;
	DrawRectangle(troughPos.x, troughPos.y, troughWidth, GUI_BAR_HEIGHT, GUI_BAR_CHANNEL);

	const int filledWidth = static_cast<int>(static_cast<float>(troughWidth) * ratio);
	ctx.renderer->draw_tile_screen_color_rect(troughPos, filledWidth, GUI_BAR_HEIGHT, tileConfig.get("GUI_BAR_FILL"), fillTint);

	// The rim is stretched the length of the bar and the plates are drawn at their own
	// size, which is why every piece of ornament is on a plate and the rim carries none.
	ctx.renderer->draw_tile_screen_color_rect(pos, width, GUI_BAR_HEIGHT, tileConfig.get("GUI_BAR_RIM"), GUI_UNTINTED);
	ctx.renderer->draw_tile_screen_sized(pos, tileConfig.get("GUI_BAR_CAP_L"), GUI_BARS_CELL_SIZE);
	ctx.renderer->draw_tile_screen_sized(
		Vector2D{ pos.x + width - GUI_BARS_CELL_SIZE, pos.y }, tileConfig.get("GUI_BAR_CAP_R"), GUI_BARS_CELL_SIZE);
}

// A bar's reading, centred on it and never left of it. The shadow under the text is what
// keeps it legible: the fill behind it is green, yellow or red by turns, and white on
// yellow is the pair that fails.
static void draw_bar_reading(
	const GameContext& ctx,
	Vector2D barPos,
	int barWidth,
	std::string_view reading,
	ColorPairId readingColor)
{
	const int textWidth = ctx.renderer->measure_text(reading);
	const int textX = barPos.x + std::max(0, (barWidth - textWidth) / 2);
	const int textY = barPos.y + (GUI_BAR_HEIGHT - ctx.renderer->get_font_size()) / 2;
	ctx.renderer->draw_text_color(Vector2D{ textX + 1, textY + 1 }, reading, GUI_TEXT_SHADOW);
	ctx.renderer->draw_text(Vector2D{ textX, textY }, reading, readingColor);
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
	std::string remainingText(part.text);
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
			part.color);
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
	assert(ctx.tileConfig && "Gui::gui_render: called without a tileConfig");
	assert(ctx.renderer && "Gui::gui_render called without a renderer");

	const int tileSize = ctx.renderer->get_tile_size();
	const int panelWidth = ctx.renderer->get_screen_width();
	const int reserveRows = ctx.renderer->get_gui_reserve_rows();
	const int baseY = hud_base_y(*ctx.renderer);
	const int ph = ctx.renderer->get_screen_height() - baseY;
	const int div1 = hud_divider1_x(panelWidth);
	const int div2 = hud_divider2_x(panelWidth);

	// Anchored to the screen edge, since whole tiles rarely fill the width exactly.
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

	// ---- Bottom edge -------------------------------------------------------
	// Drawn last, as draw_frame_pixels does, so the corners sit over the ends of the runs.
	const int bottomY = baseY + (reserveRows - 1) * tileSize;
	for (int col = tileSize; col < rightEdgeX; col += tileSize)
	{
		ctx.renderer->draw_tile_screen(Vector2D{ col, bottomY }, tileConfig.get("GUI_FRAME_B"));
	}
	ctx.renderer->draw_tile_screen(Vector2D{ 0, bottomY }, tileConfig.get("GUI_FRAME_BL"));
	ctx.renderer->draw_tile_screen(Vector2D{ rightEdgeX, bottomY }, tileConfig.get("GUI_FRAME_BR"));

	// ---- The two internal dividers ----------------------------------------
	// Each rail spans the interior between the two rules, so it covers every text row.
	const int interiorTop = baseY + gui_frame_top_rule(tileSize);
	const int interiorBottom = baseY + reserveRows * tileSize - gui_frame_bottom_rule(tileSize);
	for (const int dividerX : { div1, div2 })
	{
		ctx.renderer->draw_tile_screen(Vector2D{ dividerX, baseY }, tileConfig.get("GUI_FRAME_T"));
		for (int railY = interiorTop; railY < interiorBottom; railY += tileSize)
		{
			// The last tile is pulled up to sit flush on the bottom rule.
			// The rail art is uniform down its length, so the overlap leaves no seam.
			const int tileY = std::min(railY, interiorBottom - tileSize);
			// GUI_FRAME_V carries trim on both faces; its rail is centred in the cell, hence the offset.
			ctx.renderer->draw_tile_screen(
				Vector2D{ dividerX - DIVIDER_CELL_OFFSET, tileY },
				tileConfig.get("GUI_FRAME_V"));
		}
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
	assert(ctx.tileConfig && "Gui::render_hp_bar: called without a tileConfig");
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
		Vector2D{ hud_panel_left_margin(tileSize), rowY }, ctx.tileConfig->get("GUI_ICON_HEART"), GUI_ICON_SIZE);

	// Bar fills what is left of the panel, starting clear of the icon.
	const int barX = hud_panel_left_margin(tileSize) + GUI_ICON_SIZE + 8;
	const int barW = hud_panel_text_right(div1) - barX;

	Color filled;
	if (ratio > 0.5f)
	{
		filled = ctx.renderer->get_color_pair(ColorPairId::GREEN_BLACK).fg;
	}
	else if (ratio > 0.25f)
	{
		filled = ctx.renderer->get_color_pair(ColorPairId::YELLOW_BLACK).fg;
	}
	else
	{
		filled = ctx.renderer->get_color_pair(ColorPairId::RED_BLACK).fg;
	}

	const Vector2D barPos{ barX, rowY };
	draw_hud_bar(ctx, barPos, barW, ratio, filled);

	// No "HP" prefix: the heart icon already says it, and at three digits a prefix overruns the divider.
	draw_bar_reading(ctx, barPos, barW, std::format("{}/{}", hp, maxHp), ColorPairId::WHITE_BLACK);
}

void Gui::render_hunger_status(const GameContext& ctx)
{
	assert(ctx.hungerSystem && "Gui::render_hunger_status: called without a hungerSystem");
	assert(ctx.tileConfig && "Gui::render_hunger_status: called without a tileConfig");
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
	const ColorPairId hungerColor = ctx.hungerSystem->get_hunger_color();

	// Fullness, not hunger: this bar sits beside the health bar and has to fill
	// the same way, so a well fed creature shows a full one.
	const float ratio = ctx.hungerSystem->get_fullness_ratio();

	const int rowY = hud_text_row_y(baseY, tileSize, 1);

	// Food icon in the first column, on the same pitch as the heart above it.
	ctx.renderer->draw_tile_screen_sized(
		Vector2D{ hud_panel_left_margin(tileSize), rowY }, ctx.tileConfig->get("GUI_ICON_FOOD"), GUI_ICON_SIZE);

	// Bar fills what is left of the panel, starting clear of the icon.
	const int barX = hud_panel_left_margin(tileSize) + GUI_ICON_SIZE + 8;
	const int barW = hud_panel_text_right(div1) - barX;

	const Color filled = ctx.renderer->get_color_pair(ColorPairId::YELLOW_BLACK).fg;
	const Vector2D barPos{ barX, rowY };
	draw_hud_bar(ctx, barPos, barW, ratio, filled);

	draw_bar_reading(ctx, barPos, barW, hungerText, hungerColor);
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
	const int statsX = hud_panel_text_left(hud_divider1_x(panelWidth), tileSize);
	const int statsWidth = hud_panel_text_right(hud_divider2_x(panelWidth)) - statsX;

	if (ctx.player()->actorData.name.empty())
	{
		ctx.player()->actorData.name = "Player";
	}

	// What is missing here is on the character sheet, which carries the name, race, gold
	// and all six ability scores with their derived modifiers.

	// One stat a row, the label on the left edge and the value on the right,
	// so every value ends on the same pixel and the numbers read as one column.
	auto draw_stat = [&](int row, std::string_view label, const std::string& value)
	{
		const int y = hud_text_row_y(baseY, tileSize, row);
		ctx.renderer->draw_text(Vector2D{ statsX, y }, label, ColorPairId::WHITE_BLACK);
		const int valueWidth = ctx.renderer->measure_text(value);
		ctx.renderer->draw_text(Vector2D{ statsX + statsWidth - valueWidth, y }, value, ColorPairId::WHITE_BLACK);
	};

	// Rows 1 and 2: who this is. Both run the full width and carry no value.
	ctx.renderer->draw_text(Vector2D{ statsX, hud_text_row_y(baseY, tileSize, 0) }, ctx.renderer->fit_text_to_width(ctx.player()->actorData.name, statsWidth), ColorPairId::YELLOW_BLACK);
	ctx.renderer->draw_text(Vector2D{ statsX, hud_text_row_y(baseY, tileSize, 1) }, ctx.renderer->fit_text_to_width(ctx.player_concrete().get_class_display_name(), statsWidth), ColorPairId::YELLOW_BLACK);

	// Rows 3 and 4: the attack numbers. THAC0 is what a d20 must meet to hit Armour
	// Class 0, which is the whole of 2e attack resolution.
	draw_stat(2, "Level", std::format("{}", ctx.player()->get_level()));
	draw_stat(3, "THAC0", std::format("{}", ctx.player()->get_thaco()));

	// Rows 5 and 6: what stops a hit landing, and what it costs when one does.
	draw_stat(4, "AC", std::format("{}", ctx.player()->get_armor_class()));
	draw_stat(5, "DR", std::format("{}", ctx.player()->get_dr()));

	// Rows 7 and 8: what this character swings, and what it is carrying.
	draw_stat(6, "Attack", std::string{ ctx.player_concrete().get_equipped_weapon_damage_roll() });
	draw_stat(7, "Gold", std::format("{} gp", ctx.player_concrete().get_gold()));
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
	const int logX = hud_panel_text_left(hud_divider2_x(panelWidth), tileSize);

	const int messagesToShow = std::min(
		LOG_MAX_MESSAGES,
		static_cast<int>(ctx.messageSystem->get_stored_message_count()));

	const LogPanel panel{
		.leftEdge = logX,
		.rightEdge = hud_frame_text_right(panelWidth, tileSize),
		.baseY = baseY,
		.tileSize = tileSize,
		.rowCount = hud_text_row_count(tileSize, ctx.renderer->get_font_size())
	};

	// Newest message first, each on rows of its own; rows run out before messages do,
	// so the count above is a ceiling.
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
			Vector2D{ hud_panel_left_margin(tileSize), hud_text_row_y(baseY, tileSize, 2) },
			"CONFUSED",
			ColorPairId::RED_BLACK);
	}
}

// The interface is redrawn from game state every frame, so it holds nothing
// of its own to persist.
void Gui::save([[maybe_unused]] json& savedState)
{
}

void Gui::load([[maybe_unused]] const json& savedState)
{
}

// end of file: Gui.cpp
