// file: Gui.cpp
#include <format>
#include <algorithm>
#include <span>

#include "Gui.h"
#include "LogMessage.h"
#include "../Colors/Colors.h"
#include "../Core/GameContext.h"
#include "../Systems/MessageSystem.h"
#include "../Systems/HungerSystem.h"
#include "../ActorTypes/Player.h"
#include "../Map/Map.h"
#include "../Systems/CreatureManager.h"
#include "../Renderer/Renderer.h"
#include "../Renderer/TileId.h"
#include "../Actor/Destructible.h"
#include "../Actor/Attacker.h"

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
static int hud_div1(int vcols) { return vcols * 18 / 100; }
static int hud_div2(int vcols) { return hud_div1(vcols) + 13; }

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
	set_message(ctx.message_system->get_current_message());
	set_message_color(ctx.message_system->get_current_message_color());
}

// ---------------------------------------------------------------------------
// gui_render -- master HUD draw
// ---------------------------------------------------------------------------
void Gui::gui_render(const GameContext& ctx)
{
	if (!ctx.renderer) return;

	const int ts    = ctx.renderer->get_tile_size();
	const int vcols = ctx.renderer->get_viewport_cols();
	const int vrows = ctx.renderer->get_viewport_rows();
	const int baseY = (vrows - GUI_RESERVE_ROWS) * ts;
	const int pw    = vcols * ts;
	const int ph    = ctx.renderer->get_screen_height() - baseY;
	const int div1  = hud_div1(vcols);
	const int div2  = hud_div2(vcols);

	// ---- Background -------------------------------------------------------
	DrawRectangle(0, baseY, pw, ph, Color{ 8, 8, 16, 255 });

	// ---- Top border row (TL + T... + TR) ----------------------------------
	ctx.renderer->draw_tile_screen(0, baseY, GUI_FRAME_TL);
	for (int col = 1; col < vcols - 1; ++col)
	{
		ctx.renderer->draw_tile_screen(col * ts, baseY, GUI_FRAME_T);
	}
	ctx.renderer->draw_tile_screen((vcols - 1) * ts, baseY, GUI_FRAME_TR);

	// ---- Left and right outer edges ---------------------------------------
	for (int row = 1; row < GUI_RESERVE_ROWS; ++row)
	{
		ctx.renderer->draw_tile_screen(0,                  baseY + row * ts, GUI_FRAME_L);
		ctx.renderer->draw_tile_screen((vcols - 1) * ts,   baseY + row * ts, GUI_FRAME_R);
	}

	// ---- Divider 1: bar panel | stat panel --------------------------------
	ctx.renderer->draw_tile_screen(div1 * ts, baseY, GUI_FRAME_T);
	for (int row = 1; row < GUI_RESERVE_ROWS; ++row)
	{
		ctx.renderer->draw_tile_screen(div1 * ts, baseY + row * ts, GUI_FRAME_L);
	}

	// ---- Divider 2: stat panel | log panel --------------------------------
	ctx.renderer->draw_tile_screen(div2 * ts, baseY, GUI_FRAME_T);
	for (int row = 1; row < GUI_RESERVE_ROWS; ++row)
	{
		ctx.renderer->draw_tile_screen(div2 * ts, baseY + row * ts, GUI_FRAME_L);
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
	if (!ctx.renderer) return;

	const int ts    = ctx.renderer->get_tile_size();
	const int vcols = ctx.renderer->get_viewport_cols();
	const int vrows = ctx.renderer->get_viewport_rows();
	const int baseY = (vrows - GUI_RESERVE_ROWS) * ts;
	const int div1  = hud_div1(vcols);

	const int hp    = ctx.player->destructible->get_hp();
	const int maxHp = ctx.player->destructible->get_max_hp();
	if (maxHp <= 0) return;

	const float ratio = std::clamp(
		static_cast<float>(hp) / static_cast<float>(maxHp), 0.0f, 1.0f
	);

	const int rowY    = baseY + 1 * ts;
	const int fontOff = font_row_off(*ctx.renderer);

	// Heart icon at col 1
	ctx.renderer->draw_tile_screen(1 * ts, rowY, GUI_HEART_FULL);

	// Bar: col 2 to div1-1
	const int barX = 2 * ts;
	const int barW = (div1 - 3) * ts;
	const int barH = ts - 6;
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
	ctx.renderer->draw_bar(barX, barY, barW, barH, ratio, filled, barEmpty);

	// "HP 45/50" centered on the bar
	auto hpText = std::format("HP {}/{}", hp, maxHp);
	const int textW = ctx.renderer->measure_text(hpText);
	ctx.renderer->draw_text(
		barX + (barW - textW) / 2,
		rowY + fontOff,
		hpText,
		WHITE_BLACK_PAIR
	);
}

void Gui::render_hunger_status(const GameContext& ctx)
{
	if (!ctx.renderer) return;

	const int ts    = ctx.renderer->get_tile_size();
	const int vcols = ctx.renderer->get_viewport_cols();
	const int vrows = ctx.renderer->get_viewport_rows();
	const int baseY = (vrows - GUI_RESERVE_ROWS) * ts;
	const int div1  = hud_div1(vcols);

	if (ctx.hunger_system->get_hunger_max() <= 0) return;

	const int hungerVal = ctx.hunger_system->get_hunger_value();
	const int hungerMax = ctx.hunger_system->get_hunger_max();
	const std::string hungerText  = ctx.hunger_system->get_hunger_state_string();
	const int         hungerColor = ctx.hunger_system->get_hunger_color();

	const float ratio = std::clamp(
		static_cast<float>(hungerVal) / static_cast<float>(hungerMax), 0.0f, 1.0f
	);

	const int rowY    = baseY + 2 * ts;
	const int fontOff = font_row_off(*ctx.renderer);

	// Food icon at col 1 (from the Items/Food sheet)
	ctx.renderer->draw_tile_screen(1 * ts, rowY, make_tile(SHEET_FOOD, 0, 0));

	// Bar: col 2 to div1-1
	const int barX = 2 * ts;
	const int barW = (div1 - 3) * ts;
	const int barH = ts - 6;
	const int barY = rowY + 3;

	Color filled   = ctx.renderer->get_color_pair(YELLOW_BLACK_PAIR).fg;
	Color barEmpty = { 20, 20, 30, 255 };
	ctx.renderer->draw_bar(barX, barY, barW, barH, ratio, filled, barEmpty);

	// Hunger state centered on bar
	const int textW = ctx.renderer->measure_text(hungerText);
	ctx.renderer->draw_text(
		barX + (barW - textW) / 2,
		rowY + fontOff,
		hungerText,
		hungerColor
	);
}

// ---------------------------------------------------------------------------
// Stat panel -- middle section
// ---------------------------------------------------------------------------
void Gui::gui_print_stats(const GameContext& ctx) noexcept
{
	if (!ctx.renderer) return;

	const int ts      = ctx.renderer->get_tile_size();
	const int vcols   = ctx.renderer->get_viewport_cols();
	const int vrows   = ctx.renderer->get_viewport_rows();
	const int baseY   = (vrows - GUI_RESERVE_ROWS) * ts;
	const int statsX  = (hud_div1(vcols) + 1) * ts;
	const int fontOff = font_row_off(*ctx.renderer);

	if (ctx.player->actorData.name.empty())
	{
		ctx.player->actorData.name = "Player";
	}

	// Row 1: Name / class / level on one line
	auto nameLine = std::format(
		"{} ({} Lv.{})",
		ctx.player->actorData.name,
		ctx.player->playerClass,
		ctx.player->get_level()
	);
	ctx.renderer->draw_text(statsX, baseY + 1 * ts + fontOff, nameLine, YELLOW_BLACK_PAIR);

	// Row 2: Combat -- T0 = THAC0 abbreviation
	auto combatLine = std::format(
		"T0:{}  AC:{}  DR:{}",
		ctx.player->destructible->get_thaco(),
		ctx.player->destructible->get_armor_class(),
		ctx.player->destructible->get_dr()
	);
	ctx.renderer->draw_text(statsX, baseY + 2 * ts + fontOff, combatLine, WHITE_BLACK_PAIR);

	// Row 3: Attack roll
	auto atkLine = std::format(
		"Atk: {}", ctx.player->attacker->get_attack_damage(*ctx.player).displayRoll
	);
	ctx.renderer->draw_text(statsX, baseY + 3 * ts + fontOff, atkLine, GREEN_BLACK_PAIR);

	// Row 4: Physical attributes
	auto physLine = std::format(
		"S:{} D:{} C:{}",
		ctx.player->get_strength(),
		ctx.player->get_dexterity(),
		ctx.player->get_constitution()
	);
	ctx.renderer->draw_text(statsX, baseY + 4 * ts + fontOff, physLine, WHITE_BLACK_PAIR);

	// Row 5: Mental attributes
	auto mentLine = std::format(
		"I:{} W:{} Ch:{}",
		ctx.player->get_intelligence(),
		ctx.player->get_wisdom(),
		ctx.player->get_charisma()
	);
	ctx.renderer->draw_text(statsX, baseY + 5 * ts + fontOff, mentLine, WHITE_BLACK_PAIR);

	// Row 6: Gold
	auto goldLine = std::format("Gold: {} gp", ctx.player->get_gold());
	ctx.renderer->draw_text(statsX, baseY + 6 * ts + fontOff, goldLine, YELLOW_BLACK_PAIR);
}

// No-op: content merged into gui_print_stats
void Gui::gui_print_attrs(const GameContext& /*ctx*/) noexcept {}

// ---------------------------------------------------------------------------
// Log panel -- right section
// ---------------------------------------------------------------------------
void Gui::gui_print_log(const GameContext& ctx)
{
	if (!ctx.renderer) return;

	const int ts    = ctx.renderer->get_tile_size();
	const int vcols = ctx.renderer->get_viewport_cols();
	const int vrows = ctx.renderer->get_viewport_rows();
	const int baseY = (vrows - GUI_RESERVE_ROWS) * ts;
	const int logX  = (hud_div2(vcols) + 1) * ts;

	const int messagesToShow = std::min(
		LOG_MAX_MESSAGES,
		static_cast<int>(ctx.message_system->get_stored_message_count())
	);

	for (int i = 0; i < messagesToShow; ++i)
	{
		const std::vector<LogMessage>& parts =
			ctx.message_system->get_attack_message_at(
				ctx.message_system->get_stored_message_count() - 1 - i
			);

		int x = logX;
		const int y = baseY + (1 + i) * ts;
		int curX = 0;

		for (const auto& part : parts)
		{
			ctx.renderer->draw_text(x + curX, y, part.logMessageText, part.logMessageColor);
			curX += ctx.renderer->measure_text(part.logMessageText) + 2;
		}
	}
}

// ---------------------------------------------------------------------------
// Status effects (bar panel row 3)
// ---------------------------------------------------------------------------
void Gui::render_player_status(const GameContext& ctx)
{
	if (!ctx.renderer) return;

	const int ts      = ctx.renderer->get_tile_size();
	const int vrows   = ctx.renderer->get_viewport_rows();
	const int baseY   = (vrows - GUI_RESERVE_ROWS) * ts;
	const int fontOff = font_row_off(*ctx.renderer);

	if (ctx.player->has_state(ActorState::IS_CONFUSED))
	{
		ctx.renderer->draw_text(
			2 * ts,
			baseY + 3 * ts + fontOff,
			"CONFUSED",
			RED_BLACK_PAIR
		);
	}
}

void Gui::gui(GameContext& /*ctx*/) noexcept {}

void Gui::renderMouseLook(const GameContext& /*ctx*/) {}

void Gui::save(json& /*j*/) {}

void Gui::load(const json& /*j*/) {}

// end of file: Gui.cpp
