#pragma once

#include <curses.h>

class Player;
struct GameContext;

// - Handles character sheet display
class CharacterSheetUI
{
public:
    // Display the character sheet for the given player
    static void display_character_sheet(const Player& player, GameContext& ctx);

private:
    // UI Layout constants
    static constexpr int WINDOW_HEIGHT = 30;
    static constexpr int WINDOW_WIDTH = 120;
    static constexpr int WINDOW_Y = 0;
    static constexpr int WINDOW_X = 0;

    // Display sections
    static void display_basic_info(WINDOW* window, const Player& player);
    static void display_experience_info(WINDOW* window, const Player& player, GameContext& ctx);
    static void display_attributes(WINDOW* window, const Player& player, GameContext& ctx);
    static void display_combat_stats(WINDOW* window, const Player& player, GameContext& ctx);
    static void display_equipment_info(WINDOW* window, const Player& player, GameContext& ctx);
    static void display_right_panel_info(WINDOW* window, const Player& player, GameContext& ctx);
    static void display_constitution_effects(WINDOW* window, const Player& player, GameContext& ctx);
    static void display_strength_effects(WINDOW* window, const Player& player, GameContext& ctx);

    // Helper methods
    static int get_strength_hit_modifier(const Player& player, GameContext& ctx);
    static int get_strength_damage_modifier(const Player& player, GameContext& ctx);
    static int get_constitution_bonus(const Player& player, GameContext& ctx);
    static void cleanup_and_restore();
};
