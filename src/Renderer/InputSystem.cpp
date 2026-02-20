// file: InputSystem.cpp
#include "InputSystem.h"

void InputSystem::poll()
{
    current_key = GameKey::NONE;
    char_input  = 0;
    resized     = false;

    // ALT+ENTER fullscreen toggle -- checked before the general enter path
    if (IsKeyPressed(KEY_ENTER) && (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)))
    {
        ToggleFullscreen();
        return;
    }

    if (IsWindowResized())
    {
        resized     = true;
        current_key = GameKey::WINDOW_RESIZE;
        return;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))  { current_key = GameKey::MOUSE_LEFT;  return; }
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) { current_key = GameKey::MOUSE_RIGHT; return; }

    // Controlled key-repeat: if a repeatable key is still held and the timer
    // has elapsed, fire it again without waiting for a new IsKeyPressed event.
    if (held_raylib_key != 0)
    {
        if (IsKeyDown(held_raylib_key))
        {
            double now = GetTime();
            if (now - hold_start >= REPEAT_DELAY && now - last_repeat >= REPEAT_INTERVAL)
            {
                current_key = held_game_key;
                char_input  = held_char_input;
                last_repeat = now;
                return;
            }
        }
        else
        {
            held_raylib_key = 0;
            held_game_key   = GameKey::NONE;
            held_char_input = 0;
        }
    }

    bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

    // Helper: register a new key press.
    // repeatable=true  -- holds the key for timed repeat (movement, wait)
    // repeatable=false -- fires once only (action keys)
    auto register_key = [&](int raylib_key, GameKey gk, int ch, bool repeatable)
    {
        current_key = gk;
        char_input  = ch;
        if (repeatable)
        {
            held_raylib_key = raylib_key;
            held_game_key   = gk;
            held_char_input = ch;
            hold_start      = GetTime();
            last_repeat     = GetTime();
        }
        else
        {
            held_raylib_key = 0;
            held_game_key   = GameKey::NONE;
            held_char_input = 0;
        }
    };

    // Arrow keys and special keys
    if (IsKeyPressed(KEY_UP))        { register_key(KEY_UP,        GameKey::UP,        0,   true);  return; }
    if (IsKeyPressed(KEY_DOWN))      { register_key(KEY_DOWN,      GameKey::DOWN,      0,   true);  return; }
    if (IsKeyPressed(KEY_LEFT))      { register_key(KEY_LEFT,      GameKey::LEFT,      0,   true);  return; }
    if (IsKeyPressed(KEY_RIGHT))     { register_key(KEY_RIGHT,     GameKey::RIGHT,     0,   true);  return; }
    if (IsKeyPressed(KEY_ENTER))     { register_key(KEY_ENTER,     GameKey::ENTER,     0,   false); return; }
    if (IsKeyPressed(KEY_ESCAPE))    { register_key(KEY_ESCAPE,    GameKey::ESCAPE,    0,   false); return; }
    if (IsKeyPressed(KEY_TAB))       { register_key(KEY_TAB,       GameKey::TAB,       0,   false); return; }
    if (IsKeyPressed(KEY_SPACE))     { register_key(KEY_SPACE,     GameKey::SPACE,     0,   false); return; }
    if (IsKeyPressed(KEY_BACKSPACE)) { register_key(KEY_BACKSPACE, GameKey::BACKSPACE, 0,   false); return; }

    // WASD movement and diagonals (repeatable)
    if (IsKeyPressed(KEY_W)) { register_key(KEY_W, GameKey::W, 'w', true);  return; }
    if (IsKeyPressed(KEY_A)) { register_key(KEY_A, GameKey::A, 'a', true);  return; }
    if (IsKeyPressed(KEY_S)) { register_key(KEY_S, GameKey::S, 's', true);  return; }
    if (IsKeyPressed(KEY_D)) { register_key(KEY_D, GameKey::D, 'd', true);  return; }
    if (IsKeyPressed(KEY_Q)) { register_key(KEY_Q, GameKey::Q, 'q', true);  return; }
    if (IsKeyPressed(KEY_E)) { register_key(KEY_E, GameKey::E, 'e', true);  return; }
    if (IsKeyPressed(KEY_Z)) { register_key(KEY_Z, GameKey::Z, 'z', true);  return; }

    // Action keys (non-repeatable)
    if (IsKeyPressed(KEY_P)) { register_key(KEY_P, GameKey::PICK,       'p', false); return; }
    if (IsKeyPressed(KEY_L)) { register_key(KEY_L, GameKey::DROP,       'l', false); return; }
    if (IsKeyPressed(KEY_I)) { register_key(KEY_I, GameKey::INVENTORY,  'i', false); return; }
    if (IsKeyPressed(KEY_O)) { register_key(KEY_O, GameKey::OPEN_DOOR,  'o', false); return; }
    if (IsKeyPressed(KEY_K)) { register_key(KEY_K, GameKey::CLOSE_DOOR, 'k', false); return; }
    if (IsKeyPressed(KEY_R)) { register_key(KEY_R, GameKey::REST,       'r', false); return; }
    if (IsKeyPressed(KEY_U)) { register_key(KEY_U, GameKey::USE,        'u', false); return; }
    if (IsKeyPressed(KEY_X)) { register_key(KEY_X, GameKey::TEST_COMMAND, 'x', false); return; }
    if (IsKeyPressed(KEY_M)) { register_key(KEY_M, GameKey::REVEAL,     'm', false); return; }
    if (IsKeyPressed(KEY_N)) { register_key(KEY_N, GameKey::REGEN,      'n', false); return; }

    // Shift-ambiguous keys
    if (IsKeyPressed(KEY_C))
    {
        if (shift) register_key(KEY_C, GameKey::CAST,             'C', false);
        else       register_key(KEY_C, GameKey::C,                'c', true);
        return;
    }
    if (IsKeyPressed(KEY_H))
    {
        if (shift) register_key(KEY_H, GameKey::HIDE,             'H', false);
        else       register_key(KEY_H, GameKey::WAIT,             'h', true);
        return;
    }
    if (IsKeyPressed(KEY_T))
    {
        if (shift) register_key(KEY_T, GameKey::TOGGLE_GRIP,      'T', false);
        else       register_key(KEY_T, GameKey::TARGET,           't', false);
        return;
    }
    if (IsKeyPressed(KEY_B))
    {
        if (shift) register_key(KEY_B, GameKey::ITEM_DISTRIBUTION, 'B', false);
        else       register_key(KEY_B, GameKey::DEBUG,             'b', false);
        return;
    }

    // Shift+symbol keys (keyboard-layout-dependent) and char_input for text fields
    int ch = GetCharPressed();
    if (ch > 0)
    {
        char_input = ch;
        switch (ch)
        {
        case '@': current_key = GameKey::CHAR_SHEET; return;
        case '>': current_key = GameKey::DESCEND;    return;
        case '?': current_key = GameKey::HELP;       return;
        case '~': current_key = GameKey::QUIT;       return;
        default:  break;
        }
    }
}

Vector2D InputSystem::get_mouse_tile(int tile_size) const
{
    if (tile_size <= 0)
        return Vector2D{ 0, 0 };

    ::Vector2 mouse_pos = GetMousePosition();
    return Vector2D{
        static_cast<int>(mouse_pos.x) / tile_size,   // x = screen col
        static_cast<int>(mouse_pos.y) / tile_size    // y = screen row
    };
}

// end of file: InputSystem.cpp
