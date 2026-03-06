#pragma once
#include <string>
#include <vector>

#include "../Actor/Creature.h"
#include "../Renderer/Renderer.h"
#include "../Utils/Vector2D.h"

struct GameContext;

// Disguise data for Mimic class
struct Disguise
{
	TileRef tile{};
	std::string name{};
	int color{};
};

// Mimic has unique disguise logic — kept as a class
class Mimic : public Creature
{
public:
	Mimic(Vector2D position, GameContext& ctx);

	std::vector<Disguise> get_possible_disguises() const;

private:
	void init_disguises();

	std::vector<Disguise> possibleDisguises;
};
