#pragma once

#include "../Actor/Creature.h"
#include "../Utils/Vector2D.h"

struct GameContext;

// Mimic: construction shell only.
// All behavioral state (possibleDisguises, isDisguised, etc.) lives in AiMimic.
// The dynamic_cast-to-Mimic* pattern has been eliminated -- AiMimic operates on Creature& directly.
class Mimic : public Creature
{
public:
	Mimic(Vector2D position, GameContext& ctx);
};
