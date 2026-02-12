// InputHandler.cpp - Handles all input processing (keyboard/mouse)

#include "InputHandler.h"
#include <iostream>

extern "C" int pdc_tileset_anim_ms;

InputHandler::InputHandler() = default;

void InputHandler::key_store() noexcept
{
    std::clog << "storing key" << std::endl;
    lastKey = keyPress;
}

void InputHandler::key_listen() noexcept
{
    std::clog << "getting key" << std::endl;
    int anim_timeout = pdc_tileset_anim_ms > 0 ? pdc_tileset_anim_ms : -1;
    timeout(anim_timeout);
    keyPress = getch();
    animationTick = (keyPress == ERR);
    if (keyPress == KEY_RESIZE)
    {
        resize_term(0, 0);
        screenResized = true;
    }
}

bool InputHandler::mouse_moved() const noexcept
{
    // Simple approach: just return true and let get_mouse_position handle the detection
    return true;
}

Vector2D InputHandler::get_mouse_position() noexcept
{
    // Direct mouse position reading
    request_mouse_pos();
    currentMousePos.x = Mouse_status.x;
    currentMousePos.y = Mouse_status.y;
    return currentMousePos;
}

Vector2D InputHandler::get_mouse_position_old() const noexcept
{
    return lastMousePos;
}

void InputHandler::update_mouse_position() noexcept
{
    // Check for mouse events without blocking
    nodelay(stdscr, TRUE);
    int ch = getch();
    nodelay(stdscr, FALSE);
    
    if (ch == KEY_MOUSE)
    {
        lastMousePos = currentMousePos;
        request_mouse_pos();
        currentMousePos.x = Mouse_status.x;
        currentMousePos.y = Mouse_status.y;
    }
    // Note: We don't need to ungetch if it's not KEY_MOUSE since we used nodelay
}
