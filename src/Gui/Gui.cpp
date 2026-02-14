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
#include "../Actor/Destructible.h"
#include "../Actor/Attacker.h"

constexpr int BAR_WIDTH = 15;
constexpr int LOG_MAX_MESSAGES = 5;

void Gui::add_display_message(const std::vector<LogMessage>& message)
{
	displayMessages.push_back(message);
}

void Gui::render_messages() noexcept
{
}

void Gui::gui_init() noexcept
{
}

void Gui::gui_shutdown() noexcept
{
}

void Gui::gui_update(GameContext& ctx)
{
	set_message(ctx.message_system->get_current_message());
	set_message_color(ctx.message_system->get_current_message_color());
}

void Gui::gui_render(const GameContext& ctx)
{
	if (!ctx.renderer) return;

	int ts = ctx.renderer->get_tile_size();
	int vcols = ctx.renderer->get_viewport_cols();
	int vrows = ctx.renderer->get_viewport_rows();
	int baseY = (vrows - GUI_RESERVE_ROWS) * ts;
	int pw = vcols * ts;
	int ph = ctx.renderer->get_screen_height() - baseY;

	// Dark background for GUI area
	ColorPair bgPair = ctx.renderer->get_color_pair(WHITE_BLUE_PAIR);
	DrawRectangle(0, baseY, pw, ph, bgPair.bg);

	// Border line at top of GUI
	ColorPair borderPair = ctx.renderer->get_color_pair(WHITE_BLACK_PAIR);
	DrawLine(0, baseY, pw, baseY, borderPair.fg);

	render_hp_bar(ctx);
	render_hunger_status(ctx);
	gui_print_stats(ctx);
	gui_print_attrs(ctx);
	gui_print_log(ctx);
	render_player_status(ctx);
}

void Gui::render_player_status(const GameContext& ctx)
{
	if (!ctx.renderer) return;

	int ts = ctx.renderer->get_tile_size();
	int vrows = ctx.renderer->get_viewport_rows();
	int baseY = (vrows - GUI_RESERVE_ROWS) * ts;
	int stats_col = BAR_WIDTH + 2;

	if (ctx.player->has_state(ActorState::IS_CONFUSED))
	{
		ctx.renderer->draw_text(stats_col * ts, baseY + 6 * ts, "CONFUSED", RED_BLACK_PAIR);
	}
}

void Gui::gui_print_log(const GameContext& ctx)
{
	if (!ctx.renderer) return;

	int ts = ctx.renderer->get_tile_size();
	int vcols = ctx.renderer->get_viewport_cols();
	int vrows = ctx.renderer->get_viewport_rows();
	int baseY = (vrows - GUI_RESERVE_ROWS) * ts;

	// Log starts at roughly 55% across the viewport
	int log_x = (vcols * 55 / 100) * ts;

	int messagesToShow = std::min(
		LOG_MAX_MESSAGES,
		static_cast<int>(ctx.message_system->get_stored_message_count())
	);

	for (int i = 0; i < messagesToShow; i++)
	{
		const std::vector<LogMessage>& messageParts =
			ctx.message_system->get_attack_message_at(
				ctx.message_system->get_stored_message_count() - 1 - i
			);

		int x = log_x;
		int y = baseY + (1 + i) * ts;
		int currentX = 0;

		for (const auto& part : messageParts)
		{
			ctx.renderer->draw_text(
				x + currentX,
				y,
				part.logMessageText,
				part.logMessageColor
			);
			currentX += ctx.renderer->measure_text(part.logMessageText) + 2;
		}
	}
}

void Gui::gui_print_stats(const GameContext& ctx) noexcept
{
	if (!ctx.renderer) return;

	int ts = ctx.renderer->get_tile_size();
	int vrows = ctx.renderer->get_viewport_rows();
	int baseY = (vrows - GUI_RESERVE_ROWS) * ts;
	int stats_col = BAR_WIDTH + 2;

	if (ctx.player->actorData.name.empty())
	{
		ctx.player->actorData.name = "Player";
	}

	auto nameStr = std::format("{}", ctx.player->actorData.name);
	ctx.renderer->draw_text(1 * ts, baseY + 1 * ts, nameStr, WHITE_BLACK_PAIR);

	int hp = ctx.player->destructible->get_hp();
	int hpMax = ctx.player->destructible->get_max_hp();
	auto hpStr = std::format("HP:{}/{}", hp, hpMax);
	ctx.renderer->draw_text(stats_col * ts, baseY + 0 * ts, hpStr, WHITE_BLACK_PAIR);

	auto rollStr = std::format("Atk:{}", ctx.player->attacker->get_attack_damage(*ctx.player).displayRoll);
	ctx.renderer->draw_text(stats_col * ts, baseY + 1 * ts, rollStr, WHITE_BLACK_PAIR);

	int dr = ctx.player->destructible->get_dr();
	auto drStr = std::format("DR:{}", dr);
	ctx.renderer->draw_text(stats_col * ts, baseY + 2 * ts, drStr, WHITE_BLACK_PAIR);
}

void Gui::gui_print_attrs(const GameContext& ctx) noexcept
{
	if (!ctx.renderer) return;

	int ts = ctx.renderer->get_tile_size();
	int vrows = ctx.renderer->get_viewport_rows();
	int baseY = (vrows - GUI_RESERVE_ROWS) * ts;
	int stats_col = BAR_WIDTH + 2;

	int str = ctx.player->get_strength();
	int dex = ctx.player->get_dexterity();
	int con = ctx.player->get_constitution();
	int inte = ctx.player->get_intelligence();
	int wis = ctx.player->get_wisdom();
	int cha = ctx.player->get_charisma();

	auto line1 = std::format("STR:{} DEX:{} CON:{}", str, dex, con);
	auto line2 = std::format("INT:{} WIS:{} CHA:{}", inte, wis, cha);

	ctx.renderer->draw_text(stats_col * ts, baseY + 4 * ts, line1, WHITE_BLACK_PAIR);
	ctx.renderer->draw_text(stats_col * ts, baseY + 5 * ts, line2, WHITE_BLACK_PAIR);
}

void Gui::gui(GameContext& ctx) noexcept
{
}

void Gui::render_hp_bar(const GameContext& ctx)
{
	if (!ctx.renderer) return;

	int ts = ctx.renderer->get_tile_size();
	int vrows = ctx.renderer->get_viewport_rows();
	int baseY = (vrows - GUI_RESERVE_ROWS) * ts;

	int currentHp = ctx.player->destructible->get_hp();
	int maxHp = ctx.player->destructible->get_max_hp();
	if (maxHp <= 0) return;

	float hpRatio = static_cast<float>(currentHp) / static_cast<float>(maxHp);
	if (hpRatio < 0.0f) hpRatio = 0.0f;
	if (hpRatio > 1.0f) hpRatio = 1.0f;

	// Color based on HP percentage
	Color filled;
	if (hpRatio <= 0.25f)
	{
		filled = ctx.renderer->get_color_pair(RED_BLACK_PAIR).fg;
	}
	else if (hpRatio <= 0.5f)
	{
		filled = ctx.renderer->get_color_pair(YELLOW_BLACK_PAIR).fg;
	}
	else
	{
		filled = ctx.renderer->get_color_pair(GREEN_BLACK_PAIR).fg;
	}

	Color empty = ctx.renderer->get_color_pair(WHITE_BLACK_PAIR).bg;

	int barX = 1 * ts;
	int barY = baseY;
	int barW = BAR_WIDTH * ts;
	int barH = ts;

	ctx.renderer->draw_bar(barX, barY, barW, barH, hpRatio, filled, empty);

	auto hpText = std::format("HP: {}/{}", currentHp, maxHp);
	ctx.renderer->draw_text(barX + 2, barY + (ts - ctx.renderer->get_font_size()) / 2, hpText, WHITE_BLACK_PAIR);
}

void Gui::render_hunger_status(const GameContext& ctx)
{
	if (!ctx.renderer) return;

	int ts = ctx.renderer->get_tile_size();
	int vrows = ctx.renderer->get_viewport_rows();
	int baseY = (vrows - GUI_RESERVE_ROWS) * ts;

	if (ctx.hunger_system->get_hunger_max() <= 0) return;

	int hungerValue = ctx.hunger_system->get_hunger_value();
	int hungerMax = ctx.hunger_system->get_hunger_max();
	std::string hungerText = ctx.hunger_system->get_hunger_state_string();
	int hungerColor = ctx.hunger_system->get_hunger_color();

	float hungerRatio = static_cast<float>(hungerValue) / static_cast<float>(hungerMax);
	if (hungerRatio < 0.0f) hungerRatio = 0.0f;
	if (hungerRatio > 1.0f) hungerRatio = 1.0f;

	Color filled = ctx.renderer->get_color_pair(YELLOW_BLACK_PAIR).fg;
	Color empty = ctx.renderer->get_color_pair(WHITE_BLACK_PAIR).bg;

	int barX = 1 * ts;
	int barY = baseY + 2 * ts;
	int barW = BAR_WIDTH * ts;
	int barH = ts;

	ctx.renderer->draw_bar(barX, barY, barW, barH, hungerRatio, filled, empty);
	ctx.renderer->draw_text(barX + 2, barY + (ts - ctx.renderer->get_font_size()) / 2, hungerText, hungerColor);
}

void Gui::renderMouseLook(const GameContext& ctx)
{
}

void Gui::save(json& j)
{
}

void Gui::load(const json& j)
{
}

// end of file: Gui.cpp
