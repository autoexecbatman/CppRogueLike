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
    return currentMousePos != lastMousePos;
}

Vector2D InputHandler::get_mouse_position() noexcept
{
    update_mouse_position();
    return currentMousePos;
}

Vector2D InputHandler::get_mouse_position_old() const noexcept
{
    return lastMousePos;
}

void InputHandler::update_mouse_position() noexcept
{
    lastMousePos = currentMousePos;
    request_mouse_pos();
    currentMousePos.x = Mouse_status.x;
    currentMousePos.y = Mouse_status.y;
}
