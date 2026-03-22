#pragma once

#include "../Menu/BaseMenu.h"

class Player;
struct GameContext;

class LevelUpUI : public BaseMenu
{
public:
	LevelUpUI(Player& player, int newLevel, GameContext& ctx);
	~LevelUpUI() = default;
	LevelUpUI(const LevelUpUI&) = delete;
	LevelUpUI& operator=(const LevelUpUI&) = delete;
	LevelUpUI(LevelUpUI&&) = delete;
	LevelUpUI& operator=(LevelUpUI&&) = delete;

	void menu(GameContext& ctx) override;

private:
	Player& player_ref;
	int level;

	static void draw_title(const Player& player, int level, GameContext& ctx, int& row);
	static void draw_current_stats(const Player& player, GameContext& ctx, int& row);
	static void draw_level_benefits(const Player& player, int level, GameContext& ctx, int& row);
	static void draw_next_level_info(const Player& player, GameContext& ctx, int& row);

	static bool has_thac0_improvement(const Player& player, int level);
	static int get_expected_thac0(const Player& player, int level);
};
