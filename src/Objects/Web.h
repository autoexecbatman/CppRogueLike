#pragma once

#include "../Actor/Object.h"

struct GameContext;
struct Vector2D;
class Creature;
class TileConfig;

// Web class - represents a spider web that can trap players
class Web : public Object
{
public:
	Web(Vector2D position, int strength, const TileConfig& tileConfig);

	// Web properties
	int get_strength() const { return webStrength; }
	void set_strength(int strength) { webStrength = strength; }

	// Apply web effect to a creature trying to pass through
	bool apply_effect(Creature& creature, GameContext& ctx);

	// Object virtual — triggers web trap on movement through this tile
	bool apply_movement_effect(Creature& creature, GameContext& ctx) override;

	// Destroy this web
	void destroy(GameContext& ctx);

private:
	int webStrength{ 2 }; // How strong the web is (affects escape difficulty)
};
