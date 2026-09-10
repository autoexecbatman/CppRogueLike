#pragma once

#include "SpellTile.h"

struct GameContext;
struct Vector2D;
class Creature;
class TileConfig;

// Web class - represents a spider web that can trap players
class Web : public SpellTile
{
public:
	Web(Vector2D position, int strength, const TileConfig& tileConfig);

	// Web properties
	int get_strength() const { return webStrength; }
	void set_strength(int strength) { webStrength = strength; }

	// Apply web effect to a creature trying to pass through

	EntryResult on_creature_enter(Creature& creature, GameContext& ctx) override;


	// Destroy this web
	void destroy();

private:
	int webStrength{ 2 }; // How strong the web is (affects escape difficulty)
};
