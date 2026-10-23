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
//
// An enum held as an element of an array is read with saved_enum_value,
// which takes the element itself and names the array as the field:
//
//   states.push_back(saved_enum_value(
//       state, "Actor::load", "states", "ActorState", &actor_state_from));
//   // a saved 11 throws "Actor::load: states 11 is not a valid ActorState"

#include <format>
#include <optional>
#include <stdexcept>
#include <string_view>

#include "Persistent.h"

// The enumerator one saved value names, or the refusal that says what it is
// not, naming field as where the value was read.
//
// Example:
//   saved_enum_value(json(1), "Trap::load", "state", "TrapState",
//       &trap_state_from);   // -> TrapState::DETECTED
//   // json(4) there throws "Trap::load: state 4 is not a valid TrapState"
template <typename Enum>
[[nodiscard]] Enum saved_enum_value(
	const json& saved,
	std::string_view loader,
	std::string_view field,
	std::string_view enumName,
	std::optional<Enum> (*from)(int))
{
	const int number = saved.get<int>();
	const std::optional<Enum> enumerator = from(number);
	if (!enumerator)
	{
		throw std::runtime_error(
			std::format(
				"{}: {} {} is not a valid {}",
				loader,
				field,
				number,
				enumName));
	}
	return *enumerator;
}

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
	return saved_enum_value(record.at(field), loader, field, enumName, from);
}
