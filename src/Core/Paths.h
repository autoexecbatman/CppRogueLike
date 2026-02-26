#pragma once
// file: Paths.h
//
// Single source of truth for all file paths used at runtime.
// Change a path here and it propagates everywhere.

#include <string_view>

namespace Paths
{
    inline constexpr std::string_view LOG             = "clog.txt";

    inline constexpr std::string_view DAWNLIKE_DIR    = "DawnLike";
    inline constexpr std::string_view DAWNLIKE_FONT   = "DawnLike/GUI/SDS_8x8.ttf";

    inline constexpr std::string_view PREFABS         = "data/prefabs.json";
    inline constexpr std::string_view DECOR_OVERRIDES = "data/decor_overrides.json";
    inline constexpr std::string_view TILE_LABELS     = "data/tile_labels.json";
}
