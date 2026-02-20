#pragma once

class Player;
struct GameContext;

// Handles character sheet display via raylib renderer
class CharacterSheetUI
{
public:
    static void display_character_sheet(const Player& player, GameContext& ctx);

private:
    static void display_basic_info(const Player& player, GameContext& ctx, int& row);
    static void display_experience_info(const Player& player, GameContext& ctx, int& row);
    static void display_attributes(const Player& player, GameContext& ctx, int& row);
    static void display_combat_stats(const Player& player, GameContext& ctx, int& row);
    static void display_equipment_info(const Player& player, GameContext& ctx, int& row);
    static void display_right_panel_info(const Player& player, GameContext& ctx, int& row);

    static int get_strength_hit_modifier(const Player& player, GameContext& ctx);
    static int get_strength_damage_modifier(const Player& player, GameContext& ctx);
    static int get_constitution_bonus(const Player& player, GameContext& ctx);
};
