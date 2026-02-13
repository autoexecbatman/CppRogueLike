#pragma once

#include "../Utils/Vector2D.h"

class InputSystem;

class InputHandler
{
public:
    InputHandler() = default;
    ~InputHandler() = default;

    void key_store() noexcept;
    void key_listen(InputSystem& input) noexcept;

    bool mouse_moved() const noexcept;
    Vector2D get_mouse_position(InputSystem& input, int tile_size) noexcept;
    Vector2D get_mouse_position_old() const noexcept;

    int get_current_key() const noexcept { return keyPress; }
    int get_last_key() const noexcept { return lastKey; }

    void reset_key() noexcept { keyPress = -1; animationTick = false; }

    bool is_animation_tick() const noexcept { return animationTick; }

    bool was_resized() const noexcept { return screenResized; }
    void clear_resize() noexcept { screenResized = false; }

private:
    int keyPress{-1};
    int lastKey{0};
    bool screenResized{false};
    bool animationTick{false};

    Vector2D lastMousePos{0, 0};
    Vector2D currentMousePos{0, 0};
};
