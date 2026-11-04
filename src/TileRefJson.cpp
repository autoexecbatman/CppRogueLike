// file: TileRefJson.cpp
// The tile codec. See TileRefJson.h for the shape and examples.

#include <optional>

#include <nlohmann/json.hpp>

#include "SavedEnum.h"
#include "TileRef.h"
#include "TileRefJson.h"

namespace CppRL
{

	namespace TileRefJson
	{

		std::optional<TileSheet> tile_sheet_from(int saved)
		{
			if (0 <= saved && saved < static_cast<int>(TileSheet::COUNT))
			{
				return static_cast<TileSheet>(saved);
			}
			return std::nullopt;
		}

		TileRef parse_tile(const nlohmann::json& record)
		{
			return TileRef{
				saved_enum_field(
					record,
					"TileRefJson::parse_tile",
					"sheet",
					"TileSheet",
					&tile_sheet_from),
				record.at("col").get<int>(),
				record.at("row").get<int>(),
			};
		}

		nlohmann::json encode_tile(TileRef tile)
		{
			return nlohmann::json{
				{ "sheet", static_cast<int>(tile.sheet) },
				{ "col", tile.col },
				{ "row", tile.row },
			};
		}

	} // namespace TileRefJson

} // namespace CppRL
