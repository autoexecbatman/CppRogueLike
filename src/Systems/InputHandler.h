#pragma once

#include <curses.h>
#include "../Utils/Vector2D.h"

// - Handles all input processing (keyboard/mouse)
class InputHandler
{
public:
    InputHandler();
    ~InputHandler() = default;

    // Core input methods
    void key_store() noexcept;
    void key_listen() noexcept;
    
    // Mouse handling
    bool mouse_moved() const noexcept;
    Vector2D get_mouse_position() noexcept;
    Vector2D get_mouse_position_old() const noexcept;
    
    // Input state access
    int get_current_key() const noexcept { return keyPress; }
    int get_last_key() const noexcept { return lastKey; }
    
    // Input state management
    void reset_key() noexcept { keyPress = ERR; animationTick = false; }

    // Animation tick detection (timeout with no keypress)
    bool is_animation_tick() const noexcept { return animationTick; }

    // Resize detection
    bool was_resized() const noexcept { return screenResized; }
    void clear_resize() noexcept { screenResized = false; }

private:
    int keyPress{ 0 };
    int lastKey{ 0 };
    bool screenResized{ false };
    bool animationTick{ false };
    
    // Mouse position tracking
    Vector2D lastMousePos{ 0, 0 };
    Vector2D currentMousePos{ 0, 0 };
    
    // Helper methods
    void update_mouse_position() noexcept;
};
