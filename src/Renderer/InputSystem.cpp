// file: InputSystem.cpp
#include "InputSystem.h"

void InputSystem::poll()
{
    current_key = GameKey::NONE;
    char_input = 0;
    resized = false;

    if (IsWindowResized())
    {
        resized = true;
        current_key = GameKey::WINDOW_RESIZE;
        return;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        current_key = GameKey::MOUSE_LEFT;
        return;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
    {
        current_key = GameKey::MOUSE_RIGHT;
        return;
    }

    int char_pressed = GetCharPressed();
    if (char_pressed > 0)
    {
        char_input = char_pressed;

        // Map character presses to GameKey
        switch (char_pressed)
        {
        case 'w': current_key = GameKey::W; return;
        case 's': current_key = GameKey::S; return;
        case 'a': current_key = GameKey::A; return;
        case 'd': current_key = GameKey::D; return;
        case 'q': current_key = GameKey::Q; return;
        case 'e': current_key = GameKey::E; return;
        case 'z': current_key = GameKey::Z; return;
        case 'c': current_key = GameKey::C; return;
        case 'p': current_key = GameKey::PICK; return;
        case 'l': current_key = GameKey::DROP; return;
        case 'i': current_key = GameKey::INVENTORY; return;
        case '@': current_key = GameKey::CHAR_SHEET; return;
        case '>': current_key = GameKey::DESCEND; return;
        case 't': current_key = GameKey::TARGET; return;
        case 'T': current_key = GameKey::TOGGLE_GRIP; return;
        case 'h': current_key = GameKey::WAIT; return;
        case 'o': current_key = GameKey::OPEN_DOOR; return;
        case 'k': current_key = GameKey::CLOSE_DOOR; return;
        case 'r': current_key = GameKey::REST; return;
        case '?': current_key = GameKey::HELP; return;
        case 'H': current_key = GameKey::HIDE; return;
        case 'C': current_key = GameKey::CAST; return;
        case 'u': current_key = GameKey::USE; return;
        case '~': current_key = GameKey::QUIT; return;
        case 'b': current_key = GameKey::DEBUG; return;
        case 'x': current_key = GameKey::TEST_COMMAND; return;
        case 'B': current_key = GameKey::ITEM_DISTRIBUTION; return;
        case 'm': current_key = GameKey::REVEAL; return;
        case 'n': current_key = GameKey::REGEN; return;
        default: break;
        }
    }

    int key = GetKeyPressed();
    if (key > 0)
    {
        GameKey translated = translate_raylib_key(key);
        if (translated != GameKey::NONE)
        {
            current_key = translated;
        }
    }
}

Vector2D InputSystem::get_mouse_tile(int tile_size) const
{
    if (tile_size <= 0)
    {
        return Vector2D{ 0, 0 };
    }

    ::Vector2 mouse_pos = GetMousePosition();
    return Vector2D{
        static_cast<int>(mouse_pos.x) / tile_size,
        static_cast<int>(mouse_pos.y) / tile_size
    };
}

GameKey InputSystem::translate_raylib_key(int key) const
{
    switch (key)
    {
    case KEY_UP:      return GameKey::UP;
    case KEY_DOWN:    return GameKey::DOWN;
    case KEY_LEFT:    return GameKey::LEFT;
    case KEY_RIGHT:   return GameKey::RIGHT;
    case KEY_ENTER:   return GameKey::ENTER;
    case KEY_ESCAPE:  return GameKey::ESCAPE;
    case KEY_TAB:     return GameKey::TAB;
    case KEY_SPACE:   return GameKey::SPACE;
    case KEY_BACKSPACE: return GameKey::BACKSPACE;
    default:          return GameKey::NONE;
    }
}

// end of file: InputSystem.cpp
