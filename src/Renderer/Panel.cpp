// file: Panel.cpp
#include "Panel.h"
#include "Renderer.h"

Panel::Panel(int x, int y, int w, int h)
    : panel_rect{ x, y, w, h }
{
}

void Panel::draw_text(Renderer& renderer, int row, int col, std::string_view text, int color_pair) const
{
    int tile_size = renderer.get_tile_size();
    int px = panel_rect.x + col * tile_size;
    int py = panel_rect.y + row * tile_size;
    renderer.draw_text(px, py, text, color_pair);
}

void Panel::draw_text_reversed(Renderer& renderer, int row, int col, std::string_view text, int color_pair) const
{
    int tile_size = renderer.get_tile_size();
    int px = panel_rect.x + col * tile_size;
    int py = panel_rect.y + row * tile_size;

    ColorPair pair = renderer.get_color_pair(color_pair);
    int text_w = static_cast<int>(text.size()) * tile_size;
    int text_h = tile_size;

    DrawRectangle(px, py, text_w, text_h, pair.fg);
    renderer.draw_text(px, py, text, 0);
}

void Panel::draw_box(Renderer& renderer, Color border) const
{
    DrawRectangleLines(
        panel_rect.x,
        panel_rect.y,
        panel_rect.w,
        panel_rect.h,
        border
    );
}

void Panel::fill(Renderer& renderer, Color color) const
{
    DrawRectangle(
        panel_rect.x,
        panel_rect.y,
        panel_rect.w,
        panel_rect.h,
        color
    );
}

int Panel::cols(int font_w) const
{
    if (font_w <= 0) return 0;
    return panel_rect.w / font_w;
}

int Panel::rows(int font_h) const
{
    if (font_h <= 0) return 0;
    return panel_rect.h / font_h;
}

// end of file: Panel.cpp
