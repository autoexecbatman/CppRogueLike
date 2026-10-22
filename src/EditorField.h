#pragma once

// file: EditorField.h
//
// What an editor field is: how it is edited, what it is called, and, for a plain number, which
// member of the record it shows and between which bounds the arrows step it. Each editor holds
// one table with one descriptor per field. A plain number field is then one table line, which
// the generic path below formats and adjusts. A field with its own rules is marked by having no
// plain number, and keeps its case in the editor's switch.
//
// Usage:
//
//   using Plain = PlainNumber<ItemParams>;
//   using Descriptor = FieldDescriptor<FieldId, ItemParams>;
//   static constexpr std::array<Descriptor, 2> FIELDS{ {
//       Descriptor{ FieldId::NAME, FieldKind::TEXT, "Name", std::nullopt },
//       Descriptor{ FieldId::VALUE, FieldKind::ADJUST, "Value", Plain{ &ItemParams::value, 0, 99999 } },
//   } };
//   static_assert(is_well_formed_table(FIELDS));      // entry i is field i; a plain number only on ADJUST
//
//   plain_text(*FIELDS[1].plain, params);             // -> "150", params.value printed
//   adjust_plain(*FIELDS[1].plain, params, 10);       // params.value 150 -> 160, kept within 0 to 99999
//   refuse_misrouted_field("Value", "field_adjust");  // writes both names to stderr and aborts

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

// How a field is edited.
enum class FieldKind
{
	// Enter opens the text it holds for typing.
	TEXT,
	// Enter steps it to its next value.
	TOGGLE,
	// The arrows step it down and up.
	ADJUST,
	// Enter opens the tile picker.
	TILE
};

// A number read straight from one member of the record and stepped within two bounds.
template <typename Record>
struct PlainNumber
{
	// The member the field shows and the arrows change.
	int Record::* member{ nullptr };
	// The least value the arrows may reach.
	int minimum{ 0 };
	// The greatest value the arrows may reach.
	int maximum{ 0 };
};

// One field of an editor.
template <typename Field, typename Record>
struct FieldDescriptor
{
	// Which field this is, which must equal its position in the table.
	Field field{};
	// How it is edited.
	FieldKind kind{ FieldKind::TEXT };
	// The name the field list shows.
	std::string_view label{};
	// The member and bounds of a plain number, or none for a field with its own case.
	std::optional<PlainNumber<Record>> plain{};
};

// Whether a table lists its fields in enum order, entry i being field i, and gives a plain number
// only to an ADJUST field, with its bounds in order.
//
// Example:
//   is_well_formed_table(FIELDS);   // -> true
//   // with two entries swapped     -> false, so the static_assert beside the table fails
template <typename Field, typename Record, std::size_t Count>
consteval bool is_well_formed_table(const std::array<FieldDescriptor<Field, Record>, Count>& table)
{
	for (std::size_t index = 0; index < Count; ++index)
	{
		const FieldDescriptor<Field, Record>& entry = table[index];
		if (entry.field != static_cast<Field>(index))
		{
			return false;
		}
		if (entry.plain.has_value() && (entry.kind != FieldKind::ADJUST || entry.plain->minimum > entry.plain->maximum))
		{
			return false;
		}
	}
	return true;
}

// The descriptor for one field, which a well-formed table holds at the field's own position.
//
// Example:
//   descriptor_of(FIELDS, FieldId::VALUE).label;   // -> "Value"
template <typename Field, typename Record, std::size_t Count>
[[nodiscard]] constexpr const FieldDescriptor<Field, Record>& descriptor_of(const std::array<FieldDescriptor<Field, Record>, Count>& table, Field field)
{
	const FieldDescriptor<Field, Record>& entry = table.at(static_cast<std::size_t>(field));
	assert(entry.field == field && "descriptor_of: the table holds another field at this one's position");
	return entry;
}

// The plain number's text, the member printed as it stands.
//
// Example:
//   plain_text(Plain{ &ItemParams::acBonus, -10, 10 }, params);   // params.acBonus -3 -> "-3"
template <typename Record>
[[nodiscard]] std::string plain_text(const PlainNumber<Record>& plain, const Record& record)
{
	return std::to_string(record.*(plain.member));
}

// Steps the plain number by delta and holds it within its bounds.
//
// Example, bounds 0 to 20:
//   adjust_plain(plain, record, 1);    // 20 -> 20, held at the top
//   adjust_plain(plain, record, -1);   // 20 -> 19
template <typename Record>
void adjust_plain(const PlainNumber<Record>& plain, Record& record, int delta)
{
	record.*(plain.member) = std::clamp(record.*(plain.member) + delta, plain.minimum, plain.maximum);
}

// Stops the program when a field reaches a path its descriptor does not send it to: a field
// marked as having its own case that the switch has no case for. Writes the field's label and
// the path to stderr, then aborts, so a wrongly marked descriptor fails at its first use.
//
// Example:
//   refuse_misrouted_field("Value", "ItemEditor::field_adjust");
//   // stderr: "ItemEditor::field_adjust has no case for the field 'Value'", then abort
[[noreturn]] void refuse_misrouted_field(std::string_view label, std::string_view path);

// end of file: EditorField.h
