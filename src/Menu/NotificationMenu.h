#pragma once

// NotificationMenu — a pure info popup that closes on any keypress.
// Use for one-way messages that require no selection: racial traits,
// victory screen, help text, etc.
//
// Shows a title and a list of text lines. Any key dismisses.
// Optional onClose callback fires exactly once when the player dismisses.

#include <functional>
#include <string>
#include <vector>

#include "BaseMenu.h"

struct GameContext;

class NotificationMenu : public BaseMenu
{
    std::string title{};
    std::vector<std::string> lines{};
    std::function<void(GameContext&)> onClose{};

    void draw();

public:
    NotificationMenu(
        std::string title,
        std::vector<std::string> lines,
        GameContext& ctx);
    NotificationMenu(const NotificationMenu&) = delete;
    NotificationMenu& operator=(const NotificationMenu&) = delete;
    NotificationMenu(NotificationMenu&&) = delete;
    NotificationMenu& operator=(NotificationMenu&&) = delete;

    void set_on_close(std::function<void(GameContext&)> callback);
    void menu(GameContext& ctx) override;
};
