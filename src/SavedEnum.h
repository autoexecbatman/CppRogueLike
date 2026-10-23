#pragma once

// file: SavedEnum.h
//
// An enum read from a save or a config file, refused when the number names no
// enumerator. Each loader supplies a helper answering the enumerator or
// nullopt; this reads the field, asks it, and throws std::runtime_error
// naming the loader, the field, the number and the enum.
//
// Usage:
//
//   shopType = saved_enum_field(
//       source,               // the JSON object holding the field
//       "ShopKeeper::load",   // who is loading, first in the message
//       "shop_type",          // the key read, an int on disk
//       "ShopType",           // the enum's name, last in the message
//       &shop_type_from);     // int -> std::optional<ShopType>
//   // a 6 there throws
//   // "ShopKeeper::load: shop_type 6 is not a valid ShopType"
//
// A missing field throws json::out_of_range from at(), as every required
// field does.

#include <format>
#include <optional>
#include <stdexcept>
#include <string_view>

#include "Persistent.h"

// The enumerator record[field] names, or the refusal that says what it is not.
//
// Example:
//   saved_enum_field(source, "Trap::load", "state", "TrapState",
//       &trap_state_from);   // -> TrapState::DETECTED for a saved 1
//   // a saved 4 throws "Trap::load: state 4 is not a valid TrapState"
template <typename Enum>
[[nodiscard]] Enum saved_enum_field(
	const json& record,
	std::string_view loader,
	const char* field,
	std::string_view enumName,
	std::optional<Enum> (*from)(int))
{
	const int saved = record.at(field).get<int>();
	const std::optional<Enum> value = from(saved);
	if (!value)
	{
		throw std::runtime_error(
			std::format(
				"{}: {} {} is not a valid {}",
				loader,
				field,
				saved,
				enumName));
	}
	return *value;
}
