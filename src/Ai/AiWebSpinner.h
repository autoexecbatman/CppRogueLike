#pragma once

#include "AiSpider.h"

class Creature;
struct GameContext;
struct Vector2D;

// Web spinner spider AI - can create webs to slow down players
class AiWebSpinner : public AiSpider
{
public:
	explicit AiWebSpinner(int poisonChance);

	void update(Creature& owner, GameContext& ctx) override;
	void load(const json& j) override;
	void save(json& j) override;

private:
	int webCooldown{ 0 }; // Cooldown timer for creating webs

	// Try to create a web at the current position
	bool try_create_web(Creature& owner, GameContext& ctx);

	// Check if we should create a web (based on player proximity)
	bool should_create_web(Creature& owner, GameContext& ctx);

	// Generate a complex web pattern with entities (CIRCULAR, SPIRAL, RADIAL, CHAOTIC)
	void generate_web_entities(Vector2D center, int size, GameContext& ctx);

	// Check if a position is valid for placing a web
	bool is_valid_web_position(Vector2D pos, GameContext& ctx);
};
