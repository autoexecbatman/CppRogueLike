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

// Maximum log messages shown in the HUD
constexpr int LOG_MAX_MESSAGES = 5;

// ---------------------------------------------------------------------------
// HUD column layout (all as fraction of viewport width)
//   Bar panel  : cols 1 .. (div1-1)
//   Stat panel : cols (div1+1) .. (div2-1)
//   Log panel  : cols (div2+1) .. (vcols-2)
// ---------------------------------------------------------------------------
// Bar panel ends at ~18 % of viewport width.
// Stat panel is a fixed 11-tile interior (div1 + 1 divider + 11 content + 1 divider).
// Everything to the right of div2 belongs to the combat log.
static int hud_div1(int vcols)
{
	return vcols * 18 / 100;
}
static int hud_div2(int vcols)
{
	return hud_div1(vcols) + 13;
}

// Vertical centering offset for text inside a tile-height row
static int font_row_off(const Renderer& r)
{
	return (r.get_tile_size() - r.get_font_size()) / 2;
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
	const int div1 = hud_div1(vcols);
	const int div2 = hud_div2(vcols);

	// ---- Background -------------------------------------------------------
	DrawRectangle(0, baseY, pw, ph, Color{ 8, 8, 16, 255 });

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
	ctx.renderer->draw_tile_screen(Vector2D{ div1 * tileSize, baseY }, tileConfig.get("GUI_FRAME_T"));
	for (int row = 1; row < GUI_RESERVE_ROWS; ++row)
	{
		ctx.renderer->draw_tile_screen(Vector2D{ div1 * tileSize, baseY + row * tileSize }, tileConfig.get("GUI_FRAME_L"));
	}

	// ---- Divider 2: stat panel | log panel --------------------------------
	ctx.renderer->draw_tile_screen(Vector2D{ div2 * tileSize, baseY }, tileConfig.get("GUI_FRAME_T"));
	for (int row = 1; row < GUI_RESERVE_ROWS; ++row)
	{
		ctx.renderer->draw_tile_screen(Vector2D{ div2 * tileSize, baseY + row * tileSize }, tileConfig.get("GUI_FRAME_L"));
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
	const int div1 = hud_div1(vcols);

	const int hp = ctx.player->get_hp();
	const int maxHp = ctx.player->get_max_hp();
	if (maxHp <= 0)
	{
		return;
	}

	const float ratio = std::clamp(
		static_cast<float>(hp) / static_cast<float>(maxHp), 0.0f, 1.0f);

	const int rowY = baseY + 1 * tileSize;
	const int fontOff = font_row_off(*ctx.renderer);

	// Heart icon at col 1
	ctx.renderer->draw_tile_screen(Vector2D{ 1 * tileSize, rowY }, ctx.tileConfig->get("GUI_HEART_FULL"));

	// Bar: col 2 to div1-1
	const int barX = 2 * tileSize;
	const int barW = (div1 - 3) * tileSize;
	const int barH = tileSize - 6;
	const int barY = rowY + 3;

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

	// "HP 45/50" centered on the bar
	auto hpText = std::format("HP {}/{}", hp, maxHp);
	const int textW = ctx.renderer->measure_text(hpText);
	ctx.renderer->draw_text(
		Vector2D{ barX + (barW - textW) / 2, rowY + fontOff },
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
	const int div1 = hud_div1(vcols);

	if (ctx.hungerSystem->get_hunger_max() <= 0)
	{
		return;
	}

	const int hungerVal = ctx.hungerSystem->get_hunger_value();
	const int hungerMax = ctx.hungerSystem->get_hunger_max();
	const std::string hungerText = ctx.hungerSystem->get_hunger_state_string();
	const int hungerColor = ctx.hungerSystem->get_hunger_color();

	const float ratio = std::clamp(
		static_cast<float>(hungerVal) / static_cast<float>(hungerMax), 0.0f, 1.0f);

	const int rowY = baseY + 2 * tileSize;
	const int fontOff = font_row_off(*ctx.renderer);

	// Food icon at col 1 (from the Items/Food sheet)
	ctx.renderer->draw_tile_screen(Vector2D{ 1 * tileSize, rowY }, TileRef{ TileSheet::SHEET_FOOD, 0, 0 });

	// Bar: col 2 to div1-1
	const int barX = 2 * tileSize;
	const int barW = (div1 - 3) * tileSize;
	const int barH = tileSize - 6;
	const int barY = rowY + 3;

	Color filled = ctx.renderer->get_color_pair(YELLOW_BLACK_PAIR).fg;
	Color barEmpty = { 20, 20, 30, 255 };
	ctx.renderer->draw_bar(Vector2D{ barX, barY }, barW, barH, ratio, filled, barEmpty);

	// Hunger state centered on bar
	const int textW = ctx.renderer->measure_text(hungerText);
	ctx.renderer->draw_text(
		Vector2D{ barX + (barW - textW) / 2, rowY + fontOff },
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
	const int statsX = (hud_div1(vcols) + 1) * tileSize;
	const int fontOff = font_row_off(*ctx.renderer);

	if (ctx.player->actorData.name.empty())
	{
		ctx.player->actorData.name = "Player";
	}

	// Row 1: Name / class / level on one line
	auto nameLine = std::format(
		"{} ({} Lv.{})",
		ctx.player->actorData.name,
		ctx.player->get_class_display_name(),
		ctx.player->get_level());
	ctx.renderer->draw_text(Vector2D{ statsX, baseY + 1 * tileSize + fontOff }, nameLine, YELLOW_BLACK_PAIR);

	// Row 2: Combat -- T0 = THAC0 abbreviation
	auto combatLine = std::format(
		"T0:{}  AC:{}  DR:{}",
		ctx.player->get_thaco(),
		ctx.player->get_armor_class(),
		ctx.player->get_dr());
	ctx.renderer->draw_text(Vector2D{ statsX, baseY + 2 * tileSize + fontOff }, combatLine, WHITE_BLACK_PAIR);

	// Row 3: Attack roll
	auto atkLine = std::format(
		"Atk: {}", ctx.player->get_equipped_weapon_damage_roll());
	ctx.renderer->draw_text(Vector2D{ statsX, baseY + 3 * tileSize + fontOff }, atkLine, GREEN_BLACK_PAIR);

	// Row 4: Physical attributes
	auto physLine = std::format(
		"S:{} D:{} C:{}",
		ctx.player->get_strength(),
		ctx.player->get_dexterity(),
		ctx.player->get_constitution());
	ctx.renderer->draw_text(Vector2D{ statsX, baseY + 4 * tileSize + fontOff }, physLine, WHITE_BLACK_PAIR);

	// Row 5: Mental attributes
	auto mentLine = std::format(
		"I:{} W:{} Ch:{}",
		ctx.player->get_intelligence(),
		ctx.player->get_wisdom(),
		ctx.player->get_charisma());
	ctx.renderer->draw_text(Vector2D{ statsX, baseY + 5 * tileSize + fontOff }, mentLine, WHITE_BLACK_PAIR);

	// Row 6: Gold
	auto goldLine = std::format("Gold: {} gp", ctx.player->get_gold());
	ctx.renderer->draw_text(Vector2D{ statsX, baseY + 6 * tileSize + fontOff }, goldLine, YELLOW_BLACK_PAIR);
}

// No-op: content merged into gui_print_stats
void Gui::gui_print_attrs(const GameContext& /*ctx*/) noexcept {}

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
	const int logX = (hud_div2(vcols) + 1) * tileSize;

	const int messagesToShow = std::min(
		LOG_MAX_MESSAGES,
		static_cast<int>(ctx.messageSystem->get_stored_message_count()));

	for (int i = 0; i < messagesToShow; ++i)
	{
		const std::vector<LogMessage>& parts =
			ctx.messageSystem->get_attack_message_at(
				ctx.messageSystem->get_stored_message_count() - 1 - i);

		int x = logX;
		const int y = baseY + (1 + i) * tileSize;
		int curX = 0;

		for (const auto& part : parts)
		{
			ctx.renderer->draw_text(Vector2D{ x + curX, y }, part.logMessageText, part.logMessageColor);
			curX += ctx.renderer->measure_text(part.logMessageText) + 2;
		}
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
	const int fontOff = font_row_off(*ctx.renderer);

	if (ctx.player->has_state(ActorState::IS_CONFUSED))
	{
		ctx.renderer->draw_text(
			Vector2D{ 2 * tileSize, baseY + 3 * tileSize + fontOff },
			"CONFUSED",
			RED_BLACK_PAIR);
	}
}

void Gui::gui(GameContext& /*ctx*/) noexcept {}

void Gui::renderMouseLook(const GameContext& /*ctx*/) {}

void Gui::save(json& /*j*/) {}

void Gui::load(const json& /*j*/) {}

// end of file: Gui.cpp
