// file: Gui.cpp
#include <algorithm>
#include <cassert>
#include <format>
#include <string>
#include <vector>

#include <raylib.h>

#include "../Actor/Actor.h"
#include "../Actor/Creature.h"
#include "../Colors/Colors.h"
#include "../Core/GameContext.h"
#include "../Persistent/Persistent.h"
#include "../Renderer/Renderer.h"
#include "../Systems/HungerSystem.h"
#include "../Systems/MessageSystem.h"
#include "../Systems/TileConfig.h"
#include "Gui.h"
#include "LogMessage.h"
#include "../ActorTypes/Player.h"

// Maximum log messages shown in the HUD
constexpr int LOG_MAX_MESSAGES = 5;

// The HUD lays its text on a pitch of its own rather than on the map's tile
// grid. A tile row is 64 pixels and the font is 16, so a row spent three
// quarters of itself on nothing, and six rows of stats reserved half the screen.
constexpr int GUI_TEXT_ROW_PITCH = 32;
// Gap between the frame's top edge and the first row of text.
constexpr int GUI_TEXT_TOP_INSET = 6;
// Icons sit on the same pitch as the text beside them.
constexpr int GUI_ICON_SIZE = 32;
// Height of a bar, leaving a little air inside its row.
constexpr int GUI_BAR_HEIGHT = GUI_TEXT_ROW_PITCH - 10;
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
// HUD column layout, as fractions of viewport width:
//   Bar panel  : frame's left rule .. divider 1
//   Stat panel : divider 1 .. divider 2
//   Log panel  : divider 2 .. frame's right rule
// ---------------------------------------------------------------------------
// Both dividers are pixel positions proportional to the panel, not tile columns.
// A column-aligned divider can only move in 64-pixel steps, which is coarser than
// the difference between a stats panel that fits its longest line and one that
// does not. The stat panel gets what its widest line needs; the log gets the rest,
// because it is the panel that runs out of room first.
static int hud_divider1_x(int panelWidth)
{
	return panelWidth * 20 / 100;
}
static int hud_divider2_x(int panelWidth)
{
	return panelWidth * 52 / 100;
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

// Top edge of one HUD text row, counting from zero below the frame's top edge.
static int hud_text_row_y(int baseY, int tileSize, int row)
{
	return baseY + tileSize + GUI_TEXT_TOP_INSET + row * GUI_TEXT_ROW_PITCH;
}

// How many text rows fit in the panel. The last row needs only the height of the
// font, not a whole pitch, which is why it is added back on.
static int hud_text_row_count(int tileSize, int fontSize)
{
	const int contentHeight = (GUI_RESERVE_ROWS - 1) * tileSize - GUI_TEXT_TOP_INSET - fontSize;
	return contentHeight / GUI_TEXT_ROW_PITCH + 1;
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
	const int vcols = ctx.renderer->get_viewport_cols();
	const int vrows = ctx.renderer->get_viewport_rows();
	const int baseY = (vrows - GUI_RESERVE_ROWS) * tileSize;
	const int pw = vcols * tileSize;
	const int ph = ctx.renderer->get_screen_height() - baseY;
	const int div1 = hud_divider1_x(pw);
	const int div2 = hud_divider2_x(pw);

	// ---- Background -------------------------------------------------------
	DrawRectangle(0, baseY, pw, ph, GUI_PANEL_BACKGROUND);

	// ---- Top border row (TL + T... + TR) ----------------------------------
	const auto& tileConfig = *ctx.tileConfig;
	ctx.renderer->draw_tile_screen(Vector2D{ 0, baseY }, tileConfig.get("GUI_FRAME_TL"));
	for (int col = 1; col < vcols - 1; ++col)
	{
		ctx.renderer->draw_tile_screen(Vector2D{ col * tileSize, baseY }, tileConfig.get("GUI_FRAME_T"));
	}
	ctx.renderer->draw_tile_screen(Vector2D{ (vcols - 1) * tileSize, baseY }, tileConfig.get("GUI_FRAME_TR"));

	// ---- Left and right outer edges ---------------------------------------
	for (int row = 1; row < GUI_RESERVE_ROWS; ++row)
	{
		ctx.renderer->draw_tile_screen(Vector2D{ 0, baseY + row * tileSize }, tileConfig.get("GUI_FRAME_L"));
		ctx.renderer->draw_tile_screen(Vector2D{ (vcols - 1) * tileSize, baseY + row * tileSize }, tileConfig.get("GUI_FRAME_R"));
	}

	// ---- Divider 1: bar panel | stat panel --------------------------------
	ctx.renderer->draw_tile_screen(Vector2D{ div1, baseY }, tileConfig.get("GUI_FRAME_T"));
	for (int row = 1; row < GUI_RESERVE_ROWS; ++row)
	{
		ctx.renderer->draw_tile_screen(Vector2D{ div1, baseY + row * tileSize }, tileConfig.get("GUI_FRAME_L"));
	}

	// ---- Divider 2: stat panel | log panel --------------------------------
	ctx.renderer->draw_tile_screen(Vector2D{ div2, baseY }, tileConfig.get("GUI_FRAME_T"));
	for (int row = 1; row < GUI_RESERVE_ROWS; ++row)
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
	const int vcols = ctx.renderer->get_viewport_cols();
	const int vrows = ctx.renderer->get_viewport_rows();
	const int baseY = (vrows - GUI_RESERVE_ROWS) * tileSize;
	const int div1 = hud_divider1_x(vcols * tileSize);

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
		Vector2D{ tileSize / 2, rowY }, ctx.tileConfig->get("GUI_HEART_FULL"), GUI_ICON_SIZE);

	// Bar fills what is left of the panel, starting clear of the icon.
	const int barX = tileSize / 2 + GUI_ICON_SIZE + 8;
	const int barW = hud_panel_text_right(div1) - barX;
	const int barH = GUI_BAR_HEIGHT;
	const int barY = rowY + (GUI_TEXT_ROW_PITCH - barH) / 2;

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
	auto hpText = std::format("HP {}/{}", hp, maxHp);
	const int textW = ctx.renderer->measure_text(hpText);
	const int textX = barX + std::max(0, (barW - textW) / 2);
	ctx.renderer->draw_text(
		Vector2D{ textX, rowY + (GUI_TEXT_ROW_PITCH - ctx.renderer->get_font_size()) / 2 },
		hpText,
		WHITE_BLACK_PAIR);
}

void Gui::render_hunger_status(const GameContext& ctx)
{
	assert(ctx.renderer && "Gui::render_hunger_status called without a renderer");

	const int tileSize = ctx.renderer->get_tile_size();
	const int vcols = ctx.renderer->get_viewport_cols();
	const int vrows = ctx.renderer->get_viewport_rows();
	const int baseY = (vrows - GUI_RESERVE_ROWS) * tileSize;
	const int div1 = hud_divider1_x(vcols * tileSize);

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
		Vector2D{ tileSize / 2, rowY }, TileRef{ TileSheet::SHEET_FOOD, 0, 0 }, GUI_ICON_SIZE);

	// Bar fills what is left of the panel, starting clear of the icon.
	const int barX = tileSize / 2 + GUI_ICON_SIZE + 8;
	const int barW = hud_panel_text_right(div1) - barX;
	const int barH = GUI_BAR_HEIGHT;
	const int barY = rowY + (GUI_TEXT_ROW_PITCH - barH) / 2;

	Color filled = ctx.renderer->get_color_pair(YELLOW_BLACK_PAIR).fg;
	Color barEmpty = { 20, 20, 30, 255 };
	ctx.renderer->draw_bar(Vector2D{ barX, barY }, barW, barH, ratio, filled, barEmpty);

	// Hunger state centered on the bar, and never left of it.
	const int textW = ctx.renderer->measure_text(hungerText);
	const int textX = barX + std::max(0, (barW - textW) / 2);
	ctx.renderer->draw_text(
		Vector2D{ textX, rowY + (GUI_TEXT_ROW_PITCH - ctx.renderer->get_font_size()) / 2 },
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
	const int vcols = ctx.renderer->get_viewport_cols();
	const int vrows = ctx.renderer->get_viewport_rows();
	const int baseY = (vrows - GUI_RESERVE_ROWS) * tileSize;
	const int statsX = hud_panel_text_left(hud_divider1_x(vcols * tileSize));
	const int statsWidth = hud_panel_text_right(hud_divider2_x(vcols * tileSize)) - statsX;

	if (ctx.player()->actorData.name.empty())
	{
		ctx.player()->actorData.name = "Player";
	}

	// Row 1: Name / class / level on one line
	auto nameLine = std::format(
		"{} ({} Lv.{})",
		ctx.player()->actorData.name,
		ctx.player_concrete().get_class_display_name(),
		ctx.player()->get_level());
	ctx.renderer->draw_text(Vector2D{ statsX, hud_text_row_y(baseY, tileSize, 0) }, ctx.renderer->fit_text_to_width(nameLine, statsWidth), YELLOW_BLACK_PAIR);

	// Row 2: Combat -- T0 = THAC0 abbreviation
	auto combatLine = std::format(
		"T0:{}  AC:{}  DR:{}",
		ctx.player()->get_thaco(),
		ctx.player()->get_armor_class(),
		ctx.player()->get_dr());
	ctx.renderer->draw_text(Vector2D{ statsX, hud_text_row_y(baseY, tileSize, 1) }, ctx.renderer->fit_text_to_width(combatLine, statsWidth), WHITE_BLACK_PAIR);

	// Row 3: Attack roll
	auto atkLine = std::format(
		"Atk: {}", ctx.player_concrete().get_equipped_weapon_damage_roll());
	ctx.renderer->draw_text(Vector2D{ statsX, hud_text_row_y(baseY, tileSize, 2) }, ctx.renderer->fit_text_to_width(atkLine, statsWidth), GREEN_BLACK_PAIR);

	// Row 4: Physical attributes
	auto physLine = std::format(
		"S:{} D:{} C:{}",
		ctx.player()->get_strength(),
		ctx.player()->get_dexterity(),
		ctx.player()->get_constitution());
	ctx.renderer->draw_text(Vector2D{ statsX, hud_text_row_y(baseY, tileSize, 3) }, ctx.renderer->fit_text_to_width(physLine, statsWidth), WHITE_BLACK_PAIR);

	// Row 5: Mental attributes
	auto mentLine = std::format(
		"I:{} W:{} Ch:{}",
		ctx.player()->get_intelligence(),
		ctx.player()->get_wisdom(),
		ctx.player()->get_charisma());
	ctx.renderer->draw_text(Vector2D{ statsX, hud_text_row_y(baseY, tileSize, 4) }, ctx.renderer->fit_text_to_width(mentLine, statsWidth), WHITE_BLACK_PAIR);

	// Row 6: Gold
	auto goldLine = std::format("Gold: {} gp", ctx.player()->get_gold());
	ctx.renderer->draw_text(Vector2D{ statsX, hud_text_row_y(baseY, tileSize, 5) }, ctx.renderer->fit_text_to_width(goldLine, statsWidth), YELLOW_BLACK_PAIR);
}

// ---------------------------------------------------------------------------
// Log panel -- right section
// ---------------------------------------------------------------------------
void Gui::gui_print_log(const GameContext& ctx)
{
	assert(ctx.renderer && "Gui::gui_print_log called without a renderer");

	const int tileSize = ctx.renderer->get_tile_size();
	const int vcols = ctx.renderer->get_viewport_cols();
	const int vrows = ctx.renderer->get_viewport_rows();
	const int baseY = (vrows - GUI_RESERVE_ROWS) * tileSize;
	const int logX = hud_panel_text_left(hud_divider2_x(vcols * tileSize));

	const int messagesToShow = std::min(
		LOG_MAX_MESSAGES,
		static_cast<int>(ctx.messageSystem->get_stored_message_count()));

	const LogPanel panel{
		.leftEdge = logX,
		.rightEdge = hud_frame_text_right(vcols * tileSize),
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
	const int vrows = ctx.renderer->get_viewport_rows();
	const int baseY = (vrows - GUI_RESERVE_ROWS) * tileSize;

	if (ctx.player()->has_state(ActorState::IS_CONFUSED))
	{
		ctx.renderer->draw_text(
			Vector2D{ tileSize / 2, hud_text_row_y(baseY, tileSize, 2) },
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
