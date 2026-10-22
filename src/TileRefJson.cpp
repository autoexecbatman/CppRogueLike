// file: TileRefJson.cpp
// The tile codec. See TileRefJson.h for the shape and examples.

#include "TileRefJson.h"
#include "Persistent.h"
#include "Renderer.h"

namespace TileRefJson
{

TileRef parse_tile(const json& record)
{
	return TileRef{
		static_cast<TileSheet>(record.at("sheet").get<int>()),
		record.at("col").get<int>(),
		record.at("row").get<int>(),
	};
}

json encode_tile(TileRef tile)
{
	return json{
		{ "sheet", static_cast<int>(tile.sheet) },
		{ "col", tile.col },
		{ "row", tile.row },
	};
}

} // namespace TileRefJson
