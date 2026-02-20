#pragma once

class Player;
struct GameContext;

// Handles level up screen display via raylib renderer
class LevelUpUI
{
public:
    static void display_level_up_screen(Player& player, int newLevel, GameContext& ctx);

private:
    static void draw_title(const Player& player, int level, GameContext& ctx, int& row);
    static void draw_current_stats(const Player& player, GameContext& ctx, int& row);
    static void draw_level_benefits(const Player& player, int level, GameContext& ctx, int& row);
    static void draw_next_level_info(const Player& player, GameContext& ctx, int& row);

    static bool has_thac0_improvement(const Player& player, int level);
    static int get_expected_thac0(const Player& player, int level);
};
