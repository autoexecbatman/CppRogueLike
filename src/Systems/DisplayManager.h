#pragma once

class Player;
struct GameContext;

class DisplayManager 
{
public:
    DisplayManager() = default;
    ~DisplayManager() = default;

    // Display management methods
    void display_help() const noexcept;
    void display_levelup(Player& player, int xpLevel, GameContext& ctx) const;
    void display_character_sheet(const Player& player, GameContext& ctx) const noexcept;
};
