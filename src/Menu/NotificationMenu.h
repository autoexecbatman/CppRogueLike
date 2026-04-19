#pragma once

// NotificationMenu — a pure info popup that closes on any keypress.
// Use for one-way messages that require no selection: racial traits,
// victory screen, help text, etc.
//
// Shows a title and a list of text lines. Any key dismisses.

#include <string>
#include <vector>

#include "BaseMenu.h"

struct GameContext;

class NotificationMenu : public BaseMenu
{
    std::string title{};
    std::vector<std::string> lines{};

    void draw();

public:
    NotificationMenu(
        std::string title,
        std::vector<std::string> lines,
        GameContext& ctx);
    ~NotificationMenu() override;
    NotificationMenu(const NotificationMenu&) = delete;
    NotificationMenu& operator=(const NotificationMenu&) = delete;
    NotificationMenu(NotificationMenu&&) = delete;
    NotificationMenu& operator=(NotificationMenu&&) = delete;

    void menu(GameContext& ctx) override;
};
