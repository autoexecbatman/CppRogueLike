#pragma once

class Player;
class Creature;
struct GameContext;

// - Handles level up screen display
class LevelUpUI
{
public:
    // Display the level up screen for the given player
    static void display_level_up_screen(Player& player, int newLevel, GameContext& ctx);

private:
    // UI Layout constants
    static constexpr int WINDOW_HEIGHT = 22;
    static constexpr int WINDOW_WIDTH = 60;
    static constexpr int WINDOW_Y = 2;
    static constexpr int WINDOW_X = 10;

    // Display sections
    static void display_title(void* window, const Player& player, int level);
    static void display_basic_info(void* window, const Player& player, int level);
    static void display_current_stats(void* window, const Player& player);
    static void display_level_benefits(void* window, const Player& player, int level);
    static void display_class_benefits(void* window, const Player& player, int level, int& currentLine);
    static void display_next_level_info(void* window, const Player& player, GameContext& ctx);
    static void display_continue_prompt(void* window);

    // Helper methods
    static bool has_thac0_improvement(const Player& player, int level);
    static int get_expected_thac0(const Player& player, int level);
    static void wait_for_spacebar();
    static void cleanup_and_restore(GameContext& ctx);
};
