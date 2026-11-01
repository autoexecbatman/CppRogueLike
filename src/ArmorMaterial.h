#pragma once

// file: ArmorMaterial.h
//
// What a suit of armour is made of, which the cloak of protection's limit
// reads: it "cannot function in conjunction with ... normal armor not made of
// leather" (Dungeon Master's Guide, PDF page 952). Every item record carries
// one, NONE for anything that is not body armour, and the item editor cycles
// it.
//
// Usage:
//
//   encode_armor_material(ArmorMaterial::LEATHER);   // -> "leather"
//   parse_armor_material("metal");   // -> ArmorMaterial::METAL
//   parse_armor_material("bone");   // throws: unknown armor_material 'bone'
//   next_armor_material(ArmorMaterial::METAL);   // -> ArmorMaterial::NONE

#include <algorithm>
#include <array>
#include <format>
#include <stdexcept>
#include <string_view>

namespace CppRL
{

	// What a suit of armour is made of.
	enum class ArmorMaterial
	{
		// Not body armour.
		NONE,
		// Quilted cloth and batting: padded armour.
		CLOTH,
		// Leather, or a creature's hide: leather, studded leather, hide.
		LEATHER,
		// Metal rings, scales, plates or strips, whatever they are sewn to.
		METAL,

		// How many materials there are; names no material.
		COUNT,
	};

	// One material and its name in items.json.
	struct ArmorMaterialRecord
	{
		// The material this row stands for.
		ArmorMaterial material{ ArmorMaterial::COUNT };
		// Its name in items.json and in a save.
		std::string_view key{};
	};

	// How many materials there are, as a size.
	inline constexpr std::size_t ARMOR_MATERIAL_COUNT =
		static_cast<std::size_t>(ArmorMaterial::COUNT);

	// Every ArmorMaterial, one row each in the order the enum declares them.
	// This is the only list of the materials; everything that names them reads
	// it.
	inline constexpr std::array<ArmorMaterialRecord, ARMOR_MATERIAL_COUNT>
		ARMOR_MATERIAL_TABLE{ {
			{ ArmorMaterial::NONE, "none" },
			{ ArmorMaterial::CLOTH, "cloth" },
			{ ArmorMaterial::LEATHER, "leather" },
			{ ArmorMaterial::METAL, "metal" },
		} };

	// Whether row n of the table holds material n. A material added to the enum
	// without a row leaves a defaulted row, which fails this.
	[[nodiscard]] constexpr bool is_armor_material_table_in_enum_order()
	{
		for (std::size_t row = 0; row < ARMOR_MATERIAL_TABLE.size(); ++row)
		{
			if (ARMOR_MATERIAL_TABLE[row].material !=
					static_cast<ArmorMaterial>(row) ||
				ARMOR_MATERIAL_TABLE[row].key.empty())
			{
				return false;
			}
		}
		return true;
	}

	static_assert(
		is_armor_material_table_in_enum_order(),
		"ARMOR_MATERIAL_TABLE needs one row per ArmorMaterial, in enum order");

	// The material's name in items.json.
	//
	// Example:
	//   encode_armor_material(ArmorMaterial::CLOTH);   // -> "cloth"
	[[nodiscard]] constexpr std::string_view encode_armor_material(
		ArmorMaterial material)
	{
		return ARMOR_MATERIAL_TABLE.at(static_cast<std::size_t>(material)).key;
	}

	// The material a name stands for. Throws std::runtime_error for a name no
	// row carries.
	//
	// Example:
	//   parse_armor_material("leather");   // -> ArmorMaterial::LEATHER
	//   parse_armor_material("bone");
	//   // throws: unknown armor_material 'bone'
	[[nodiscard]] inline ArmorMaterial parse_armor_material(
		std::string_view name)
	{
		const auto found = std::ranges::find(
			ARMOR_MATERIAL_TABLE,
			name,
			&ArmorMaterialRecord::key);
		if (found == ARMOR_MATERIAL_TABLE.end())
		{
			throw std::runtime_error(
				std::format("unknown armor_material '{}'", name));
		}
		return found->material;
	}

	// The next material in table order, wrapping at the end, so a cycle through
	// the editor's field reaches all of them.
	//
	// Example:
	//   next_armor_material(ArmorMaterial::NONE);   // -> ArmorMaterial::CLOTH
	//   next_armor_material(ArmorMaterial::METAL);   // -> ArmorMaterial::NONE
	[[nodiscard]] constexpr ArmorMaterial next_armor_material(
		ArmorMaterial material)
	{
		const std::size_t next = static_cast<std::size_t>(material) + 1;
		return ARMOR_MATERIAL_TABLE.at(next % ARMOR_MATERIAL_COUNT).material;
	}

} // namespace CppRL
