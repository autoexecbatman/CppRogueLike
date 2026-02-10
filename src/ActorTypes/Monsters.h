#pragma once

#include "../Actor/Actor.h"

struct GameContext;

// Disguise data for Mimic class
struct Disguise
{
    char ch;
    std::string name;
    int color;
};

// Mimic has unique disguise logic — kept as a class
class Mimic : public Creature
{
public:
    Mimic(Vector2D position, GameContext& ctx);

    std::vector<Disguise> get_possible_disguises() const;

private:
    void initDisguises();

    std::vector<Disguise> possibleDisguises;
};
