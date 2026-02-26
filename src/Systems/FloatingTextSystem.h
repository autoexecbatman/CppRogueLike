// file: FloatingTextSystem.h
#pragma once

#include <string>
#include <vector>

class Renderer;

struct FloatingEntry
{
    int world_x;
    int world_y;
    std::string text;
    unsigned char r, g, b;
    float spawn_time;
    float lifetime;
};

class FloatingTextSystem
{
public:
    void spawn_damage(
        int world_x,
        int world_y,
        int value,
        unsigned char r,
        unsigned char g,
        unsigned char b
    );

    void spawn_text(
        int world_x,
        int world_y,
        std::string text,
        unsigned char r,
        unsigned char g,
        unsigned char b,
        float lifetime
    );

    void update_and_render(const Renderer& renderer);

private:
    std::vector<FloatingEntry> entries;
};
