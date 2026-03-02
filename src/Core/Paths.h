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
inline constexpr std::string_view SAVE_FILE = "saves/game.sav";

inline constexpr std::string_view DAWNLIKE_DIR = "DawnLike";
inline constexpr std::string_view DAWNLIKE_FONT = "DawnLike/GUI/SDS_8x8.ttf";

inline constexpr std::string_view PREFABS = "data/prefabs.json";
inline constexpr std::string_view CONTENT_TILES = "data/content/tiles.json";
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
