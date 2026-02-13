#pragma once

#include <raylib.h>

// Undefine raylib color macros that conflict with game enum values
#undef LIGHTGRAY
#undef GRAY
#undef DARKGRAY
#undef YELLOW
#undef GOLD
#undef ORANGE
#undef PINK
#undef RED
#undef MAROON
#undef GREEN
#undef LIME
#undef DARKGREEN
#undef SKYBLUE
#undef BLUE
#undef DARKBLUE
#undef PURPLE
#undef VIOLET
#undef DARKPURPLE
#undef BEIGE
#undef BROWN
#undef DARKBROWN
#undef WHITE
#undef BLACK
#undef BLANK
#undef MAGENTA
#undef RAYWHITE

#include "../Utils/Vector2D.h"

enum class GameKey
{
    NONE = 0,

    // Movement
    UP,
    DOWN,
    LEFT,
    RIGHT,

    // WASD movement
    W, A, S, D,

    // Diagonals
    Q, E, Z, C,

    // Actions
    ENTER,
    ESCAPE,
    TAB,
    SPACE,
    BACKSPACE,

    // Game actions
    PICK,
    DROP,
    INVENTORY,
    CHAR_SHEET,
    DESCEND,
    TARGET,
    TOGGLE_GRIP,
    WAIT,
    OPEN_DOOR,
    CLOSE_DOOR,
    REST,
    HELP,
    HIDE,
    CAST,
    USE,
    QUIT,

    // Debug
    DEBUG,
    TEST_COMMAND,
    ITEM_DISTRIBUTION,
    REVEAL,
    REGEN,

    // Mouse
    MOUSE_LEFT,
    MOUSE_RIGHT,

    // Window
    WINDOW_RESIZE,
};

class InputSystem
{
public:
    InputSystem() = default;
    ~InputSystem() = default;

    InputSystem(const InputSystem&) = delete;
    InputSystem& operator=(const InputSystem&) = delete;
    InputSystem(InputSystem&&) = delete;
    InputSystem& operator=(InputSystem&&) = delete;

    void poll();

    [[nodiscard]] GameKey get_key() const { return current_key; }
    [[nodiscard]] int get_char_input() const { return char_input; }
    [[nodiscard]] Vector2D get_mouse_tile(int tile_size) const;
    [[nodiscard]] bool has_player_action() const { return current_key != GameKey::NONE; }
    [[nodiscard]] bool window_resized() const { return resized; }

private:
    GameKey current_key{ GameKey::NONE };
    int char_input{ 0 };
    bool resized{ false };

    [[nodiscard]] GameKey translate_raylib_key(int key) const;
};
