#pragma once
// file: Paths.h
//
// Single source of truth for all file paths used at runtime.
// Change a path here and it propagates everywhere.

#include <filesystem>
#include <string_view>

namespace Paths
{
inline constexpr std::string_view LOG      = "clog.txt";
inline constexpr std::string_view SAVE_FILE = "saves/game->sav";

inline constexpr std::string_view DAWNLIKE_DIR = "DawnLike";
// The advance is the load size, not the design: both this and SDS_6x6 measured
// 16.8 pixels a character at a load size of 16. So the narrower file buys no width
// and only changes the letterforms, and the width comes from the load size in
// main.cpp instead.
inline constexpr std::string_view DAWNLIKE_FONT = "DawnLike/GUI/SDS_8x8.ttf";

inline constexpr std::string_view PREFABS = "data/prefabs.json";
inline constexpr std::string_view CONTENT_TILES = "data/content/tiles.json";
inline constexpr std::string_view MONSTERS = "data/content/monsters.json";
inline constexpr std::string_view SPELLS   = "data/content/spells.json";
inline constexpr std::string_view BODY_PLANS = "data/content/body_plans.json";
inline constexpr std::string_view ITEMS          = "data/content/items.json";
inline constexpr std::string_view ENHANCED_RULES = "data/content/enhanced_rules.json";
inline constexpr std::string_view TILE_CONFIG = "data/tiles/tile_config.json";

// Walks upward from cwd until a directory containing "data/" is found,
// then resolves 'relative' against that root.
// Handles VS2022 / Ninja CWD mismatch transparently.
inline std::filesystem::path resolve(std::string_view relative)
{
	namespace fs = std::filesystem;
	auto dir = fs::current_path();
	for (int i = 0; i < 8; ++i)
	{
		if (fs::is_directory(dir / "data"))
			return dir / relative;
		auto parent = dir.parent_path();
		if (parent == dir)
			break;
		dir = parent;
	}
	return fs::current_path() / relative;
}
} // namespace Paths
