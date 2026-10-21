#pragma once

#include <optional>

#include "AiSpider.h"

class Creature;
struct GameContext;
struct Vector2D;

// Web spinner spider AI - can create webs to slow down players
class AiWebSpinner : public AiSpider
{
private:
	// The clock reading the next web is allowed at. Zero lets a fresh spinner spin at
	// once, which is what a cooldown starting at zero already meant.
	int webReadyTime{ 0 };

	// Try to create a web, aimed at the quarry when there is one in sight
	bool try_create_web(Creature& owner, std::optional<Vector2D> quarry, GameContext& ctx);

	// Check if we should create a web; the quarry is the player, or none once it is lost
	bool should_create_web(Creature& owner, std::optional<Vector2D> quarry, GameContext& ctx);

	// Generate a complex web pattern with entities (CIRCULAR, SPIRAL, RADIAL, CHAOTIC)
	void generate_web_entities(Vector2D center, int size, GameContext& ctx);

	// Check if a position is valid for placing a web
	bool is_valid_web_position(Vector2D pos, GameContext& ctx);

protected:
	// Type F: the victim saves against poison or dies where it stands.
	void inject_venom(Creature& owner, Creature& target, GameContext& ctx) override;

public:
	void update(Creature& owner, GameContext& ctx) override;

	[[nodiscard]] AiType get_ai_type() const noexcept override { return AiType::WEB_SPINNER; }

	void load(const json& source) override;
	void save(json& output) override;
};
