#pragma once

#include <optional>

#include "AiMonster.h"
#include "Persistent.h"

// Forward declarations
class Creature;
struct Vector2D;
struct GameContext;

// Base spider AI class - handles spider movement patterns
class AiSpider : public AiMonster
{
public:
	void update(Creature& owner, GameContext& ctx) override;

	[[nodiscard]] AiType get_ai_type() const noexcept override { return AiType::SPIDER; }
	void move_toward_player(Creature& owner, GameContext& ctx);
	void random_move(Creature& owner, GameContext& ctx);
	void load(const json& j) override;
	void save(json& j) override;

	bool has_laid_web() const { return webLaid; }
	void set_web_laid(bool status) { webLaid = status; }

protected:
	// The clock reading the ambush ends at. Read only while isAmbushing, and always set
	// when an ambush begins, so there is no reading it before it means anything.
	int ambushEndTime{ 0 };
	bool isAmbushing{ false }; // Is this spider currently in ambush mode?
	bool webLaid{ false }; // Tracks if this spider has created a web

	// Specialized spider movement pattern that prefers walls and corners
	void move_or_attack(Creature& owner, Vector2D targetPosition, GameContext& ctx) override;

	// One round of the spider's attacks: its bite, and the venom a bite that lands injects.
	//
	// Example, beside the player:
	//   bite(owner, *ctx.player(), ctx);   // attack roll, damage on a hit, then the venom
	void bite(Creature& owner, Creature& target, GameContext& ctx);

	// What this spider's venom does to a victim its bite has just landed on. This one
	// is the Monstrous Manual's hairy spider, whose weak poison penalises rather than
	// wounds; the kinds with a Table 51 poison override it.
	virtual void inject_venom(Creature& owner, Creature& target, GameContext& ctx);

	// Find the best ambush position near walls. Returns nullopt when no valid position exists.
	std::optional<Vector2D> find_ambush_position(Creature& owner, Vector2D targetPosition, GameContext& ctx);

	// Check if a position is a good ambush spot (near walls)
	bool is_good_ambush_spot(Vector2D position, GameContext& ctx);
};
