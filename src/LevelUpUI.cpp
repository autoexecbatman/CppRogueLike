// LevelUpUI.cpp - Handles level up screen display
#include <format>

#include <raylib.h>

#include "Colors.h"
#include "GameContext.h"
#include "InputSystem.h"
#include "LevelUpSystem.h"
#include "LevelUpUI.h"
#include "Player.h"
#include "Renderer.h"

// ============================================================================
// Private helpers — not visible outside this translation unit.
// ============================================================================

namespace
{

void draw_title(const Player& player, int level, GameContext& ctx, int& row)
{
	int tileSize = ctx.renderer->get_tile_size();

	ctx.renderer->draw_text(Vector2D{ tileSize, panel_text_row_y(0, tileSize, row) }, "*** LEVEL UP! ***", ColorPairId::YELLOW_BLACK);
	row++;

	ctx.renderer->draw_text(
		Vector2D{ tileSize, panel_text_row_y(0, tileSize, row) }, std::format("{} has attained level {}!", player.get_name(), level), ColorPairId::WHITE_BLACK);
	row++;

	ctx.renderer->draw_text(
		Vector2D{ tileSize, panel_text_row_y(0, tileSize, row) }, std::format("Class: {}", player.playerClass), ColorPairId::WHITE_BLACK);
	row += 2;
}

void draw_current_stats(const Player& player, GameContext& ctx, int& row)
{
	int tileSize = ctx.renderer->get_tile_size();

	ctx.renderer->draw_text(Vector2D{ tileSize, panel_text_row_y(0, tileSize, row) }, "--- CURRENT STATS ---", ColorPairId::YELLOW_BLACK);
	row++;

	ctx.renderer->draw_text(
		Vector2D{ tileSize, panel_text_row_y(0, tileSize, row) }, std::format("HP: {} / {}   THAC0: {}   AC: {}", player.get_hp(), player.get_max_hp(), player.get_thaco(), player.get_armor_class()), ColorPairId::GREEN_BLACK);
	row += 2;
}

void draw_level_benefits(const Player& player, int level, GameContext& ctx, int& row)
{
	int tileSize = ctx.renderer->get_tile_size();

	ctx.renderer->draw_text(Vector2D{ tileSize, panel_text_row_y(0, tileSize, row) }, "--- LEVEL BENEFITS ---", ColorPairId::YELLOW_BLACK);
	row++;

	if (LevelUpSystem::thac0_improves_at(player.get_creature_class(), level))
	{
		ctx.renderer->draw_text(
			Vector2D{ tileSize, panel_text_row_y(0, tileSize, row) }, std::format("THAC0 improved to {}", player.get_thaco()), ColorPairId::GREEN_BLACK);
		row++;
	}

	switch (player.playerClassState)
	{
	case Player::PlayerClassState::FIGHTER:
	{
		ctx.renderer->draw_text(Vector2D{ tileSize, panel_text_row_y(0, tileSize, row) }, "Hit Dice: d10", ColorPairId::WHITE_BLACK);
		row++;
		if (level == 7)
		{
			ctx.renderer->draw_text(
				Vector2D{ tileSize, panel_text_row_y(0, tileSize, row) }, "Special: Extra Attack (3 attacks per 2 rounds)", ColorPairId::YELLOW_BLACK);
			row++;
		}
		else if (level == 13)
		{
			ctx.renderer->draw_text(
				Vector2D{ tileSize, panel_text_row_y(0, tileSize, row) }, "Special: Extra Attack (2 attacks per round)", ColorPairId::YELLOW_BLACK);
			row++;
		}
		break;
	}
	case Player::PlayerClassState::ROGUE:
	{
		ctx.renderer->draw_text(Vector2D{ tileSize, panel_text_row_y(0, tileSize, row) }, "Hit Dice: d6", ColorPairId::WHITE_BLACK);
		row++;
		int newMult = LevelUpSystem::calculate_backstab_multiplier(level);
		int oldMult = LevelUpSystem::calculate_backstab_multiplier(level - 1);
		if (newMult > oldMult)
		{
			ctx.renderer->draw_text(
				Vector2D{ tileSize, panel_text_row_y(0, tileSize, row) }, std::format("Special: Backstab multiplier increased to x{}", newMult), ColorPairId::YELLOW_BLACK);
			row++;
		}
		break;
	}
	case Player::PlayerClassState::CLERIC:
	{
		ctx.renderer->draw_text(Vector2D{ tileSize, panel_text_row_y(0, tileSize, row) }, "Hit Dice: d8", ColorPairId::WHITE_BLACK);
		row++;
		if (level == 3 || level == 5 || level == 7 || level == 9)
		{
			ctx.renderer->draw_text(
				Vector2D{ tileSize, panel_text_row_y(0, tileSize, row) }, "Special: Turn Undead ability improved", ColorPairId::YELLOW_BLACK);
			row++;
		}
		break;
	}
	case Player::PlayerClassState::WIZARD:
	{
		ctx.renderer->draw_text(Vector2D{ tileSize, panel_text_row_y(0, tileSize, row) }, "Hit Dice: d4", ColorPairId::WHITE_BLACK);
		row++;
		if ((level % 2 == 1) && level > 1)
		{
			int spellLevel = (level + 1) / 2;
			if (spellLevel <= 9)
			{
				ctx.renderer->draw_text(
					Vector2D{ tileSize, panel_text_row_y(0, tileSize, row) }, std::format("Special: Level {} spells now available", spellLevel), ColorPairId::YELLOW_BLACK);
				row++;
			}
		}
		break;
	}
	default:
		break;
	}

	row++;
}

void draw_next_level_info(const Player& player, GameContext& ctx, int& row)
{
	int tileSize = ctx.renderer->get_tile_size();

	int nextLevelXP = player.get_next_level_xp();

	ctx.renderer->draw_text(Vector2D{ tileSize, panel_text_row_y(0, tileSize, row) }, "--- NEXT LEVEL ---", ColorPairId::YELLOW_BLACK);
	row++;

	ctx.renderer->draw_text(
		Vector2D{ tileSize, panel_text_row_y(0, tileSize, row) }, std::format("XP needed for next level: {}", nextLevelXP), ColorPairId::WHITE_BLACK);
	row += 2;
}

} // anonymous namespace

// ============================================================================
// LevelUpUI
// ============================================================================

LevelUpUI::LevelUpUI(Player& player, int newLevel)
	: playerRef(player), level(newLevel)
{
}

void LevelUpUI::menu(GameContext& ctx)
{
	ctx.inputSystem->poll();
	GameKey key = ctx.inputSystem->get_key();
	if (key == GameKey::SPACE || key == GameKey::ESCAPE || key == GameKey::ENTER)
	{
		menu_set_run_false();
		return;
	}

	ctx.renderer->begin_frame();

	int row = 0;
	draw_title(playerRef, level, ctx, row);
	draw_current_stats(playerRef, ctx, row);
	draw_level_benefits(playerRef, level, ctx, row);
	draw_next_level_info(playerRef, ctx, row);

	int tileSize = ctx.renderer->get_tile_size();
	ctx.renderer->draw_text(Vector2D{ tileSize, panel_text_row_y(0, tileSize, row) }, "Press [SPACE] to continue", ColorPairId::CYAN_BLACK);

	ctx.renderer->end_frame();
}
