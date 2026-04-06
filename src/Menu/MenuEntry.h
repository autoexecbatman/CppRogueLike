#pragma once

// MenuEntry — a single selectable item in a vertical list menu.
// Holds the label shown to the user, an optional hotkey, and the
// command executed when selected. Shared by ListMenu and MenuTrade.

#include <functional>
#include <string>

struct GameContext;

struct MenuEntry
{
    std::string label{};
    char hotkey{ 0 };
    std::function<void(GameContext&)> command{};
};
