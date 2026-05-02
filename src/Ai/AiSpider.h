#pragma once

#include <optional>

#include "../Persistent/Persistent.h"
#include "AiMonster.h"

// Forward declarations
class Creature;
struct Vector2D;
struct GameContext;

// Base spider AI class - handles spider movement patterns
class AiSpider : public AiMonster
{
public:
	explicit AiSpider(int poisonChance);

	void update(Creature& owner, GameContext& ctx) override;
	void move_toward_player(Creature& owner, GameContext& ctx);
	void random_move(Creature& owner, GameContext& ctx);
	void load(const json& j) override;
	void save(json& j) override;

	bool has_laid_web() const { return webLaid; }
	void set_web_laid(bool status) { webLaid = status; }

protected:
	int ambushCounter{ 0 }; // Counter for ambush behavior
	bool isAmbushing{ false }; // Is this spider currently in ambush mode?
	int poisonCooldown{ 0 }; // Cooldown for poison attacks
	int poisonChance; // Per-type poison hit probability (0-100), set at construction
	bool webLaid{ false }; // Tracks if this spider has created a web

	// Specialized spider movement pattern that prefers walls and corners
	void move_or_attack(Creature& owner, Vector2D targetPosition, GameContext& ctx) override;

	// Check if the spider can attempt a poison attack
	bool can_poison_attack(Creature& owner, GameContext& ctx);

	// Perform a poison attack on the target
	void poison_attack(Creature& owner, Creature& target, GameContext& ctx);

	// Find the best ambush position near walls. Returns nullopt when no valid position exists.
	std::optional<Vector2D> find_ambush_position(Creature& owner, Vector2D targetPosition, GameContext& ctx);

	// Check if a position is a good ambush spot (near walls)
	bool is_good_ambush_spot(Vector2D position, GameContext& ctx);
};
