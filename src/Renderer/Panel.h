#pragma once

#include <string_view>
#include <raylib.h>

class Renderer;

struct PanelRect
{
    int x;
    int y;
    int w;
    int h;
};

class Panel
{
public:
    Panel() = default;
    Panel(int x, int y, int w, int h);

    void draw_text(Renderer& renderer, int row, int col, std::string_view text, int color_pair) const;
    void draw_text_reversed(Renderer& renderer, int row, int col, std::string_view text, int color_pair) const;
    void draw_box(Renderer& renderer, Color border) const;
    void fill(Renderer& renderer, Color color) const;

    [[nodiscard]] int cols(int font_w) const;
    [[nodiscard]] int rows(int font_h) const;
    [[nodiscard]] PanelRect rect() const { return panel_rect; }

private:
    PanelRect panel_rect{ 0, 0, 0, 0 };
};
