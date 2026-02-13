#include "InputHandler.h"
#include "../Renderer/InputSystem.h"

void InputHandler::key_store() noexcept
{
    lastKey = keyPress;
}

void InputHandler::key_listen(InputSystem& input) noexcept
{
    // Check for window resize
    if (input.window_resized())
    {
        screenResized = true;
        keyPress = -1;
        animationTick = false;
        return;
    }

    // Character keys first (ASCII values match Controls enum)
    int ch = input.get_char_input();
    if (ch != 0)
    {
        keyPress = ch;
        animationTick = false;
        return;
    }

    // Special keys mapped to legacy keycodes
    GameKey gk = input.get_key();
    switch (gk)
    {
    case GameKey::UP:         keyPress = 0x103; break;
    case GameKey::DOWN:       keyPress = 0x102; break;
    case GameKey::LEFT:       keyPress = 0x104; break;
    case GameKey::RIGHT:      keyPress = 0x105; break;
    case GameKey::ENTER:      keyPress = 10; break;
    case GameKey::ESCAPE:     keyPress = 27; break;
    case GameKey::TAB:        keyPress = 9; break;
    case GameKey::SPACE:      keyPress = ' '; break;
    case GameKey::MOUSE_LEFT: keyPress = 0x199; break;
    default:
        keyPress = -1;
        animationTick = true;
        return;
    }
    animationTick = false;
}

bool InputHandler::mouse_moved() const noexcept
{
    return lastMousePos.x != currentMousePos.x || lastMousePos.y != currentMousePos.y;
}

Vector2D InputHandler::get_mouse_position(InputSystem& input, int tile_size) noexcept
{
    lastMousePos = currentMousePos;
    currentMousePos = input.get_mouse_tile(tile_size);
    return currentMousePos;
}

Vector2D InputHandler::get_mouse_position_old() const noexcept
{
    return lastMousePos;
}
