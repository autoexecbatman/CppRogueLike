#pragma once
#include <string>
#include <vector>

#include "../Actor/Creature.h"
#include "../Renderer/Renderer.h"
#include "../Utils/Vector2D.h"

struct GameContext;
class ContentRegistry;

// Disguise data for Mimic class
struct Disguise
{
	TileRef tile{};
	std::string name{};
	int color{};
};

// TODO(smell): Mimic exists only because AiMimic requires a Mimic* via dynamic_cast.
// This is an LSP violation -- AiMimic::update signature says Creature& but secretly requires Mimic&.
// Root cause: AiMimic should not exist. Mimic behaviors (disguise, item consumption, reveal)
// belong as data-driven components in JSON, not hardcoded AI subclasses.
// Fix: DisguiseComponent, ConsumptionComponent, RevealComponent attached to a plain Creature.
// dynamic_cast is the symptom. AiMimic is the disease.
class Mimic : public Creature
{
public:
	Mimic(Vector2D position, GameContext& ctx);

	std::vector<Disguise> get_possible_disguises() const;

private:
	void init_disguises(ContentRegistry& registry);

	std::vector<Disguise> possibleDisguises;
};
