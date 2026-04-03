#pragma once
// file: Map/Decoration.h
//
// Static, authored dungeon decoration (barrel, crate, altar, etc.).
// Lives in a global vector; never crosses room boundaries at runtime.
// Placed at generation time, destroyed by player bumping into it.

#include <string>

#include "../Renderer/Renderer.h"
#include "../Utils/Vector2D.h"

struct Decoration
{
	Vector2D position{};
	TileRef tile{};
	std::string name{};
    int hp{ 1 };
    bool blocks_movement{ true };
	std::string loot_table_key{}; // "" = drops nothing on break
    bool is_broken{ false };
};
