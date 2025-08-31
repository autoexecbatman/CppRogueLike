// InputHandler.cpp - Handles all input processing (keyboard/mouse)

#include "InputHandler.h"
#include <iostream>

InputHandler::InputHandler() = default;

void InputHandler::key_store() noexcept
{
    std::clog << "storing key" << std::endl;
    lastKey = keyPress;
}

void InputHandler::key_listen() noexcept
{
    std::clog << "getting key" << std::endl;
    keyPress = getch();
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
