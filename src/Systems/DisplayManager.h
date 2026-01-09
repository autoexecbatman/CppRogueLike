// file: Systems/DisplayManager.h
#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

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
    void display_character_sheet(const Player& player) const noexcept;

private:
    // Private helper methods if needed
};

#endif // DISPLAY_MANAGER_H
// end of file: Systems/DisplayManager.h
