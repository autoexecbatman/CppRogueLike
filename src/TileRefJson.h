#pragma once
// file: TileRefJson.h
//
// A tile reference written as JSON and read back, in the one shape every save and
// content file uses: { "sheet": int, "col": int, "row": int }.
//
// Example:
//   const json record = TileRefJson::encode_tile(TileRef{ TileSheet::SHEET_DECOR0, 3, 7 });
//   // record -> { "sheet": <SHEET_DECOR0 as int>, "col": 3, "row": 7 }
//   TileRefJson::parse_tile(record);                // -> TileRef{ TileSheet::SHEET_DECOR0, 3, 7 }
//   TileRefJson::parse_tile(json{ { "col", 3 } });   // throws json::out_of_range: no "sheet"

#include "Persistent.h"
#include "TileRef.h"

namespace TileRefJson
{

// Reads a tile from its three fields. Every field is required, so a record missing one
// throws rather than reading as an empty tile.
//
// Example:
//   TileRefJson::parse_tile(json{ { "sheet", 0 }, { "col", 2 }, { "row", 5 } });   // -> sheet 0, col 2, row 5
[[nodiscard]] TileRef parse_tile(const json& record);

// Writes a tile as its three fields, the shape parse_tile reads.
//
// Example:
//   TileRefJson::encode_tile(TileRef{ TileSheet::SHEET_DECOR0, 3, 7 })["col"];   // -> 3
[[nodiscard]] json encode_tile(TileRef tile);

} // namespace TileRefJson
