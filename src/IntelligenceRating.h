#pragma once

#include <algorithm>
#include <array>
#include <format>
#include <stdexcept>
#include <string_view>

// file: IntelligenceRating.h
//
// The Intelligence rating every Monstrous Manual stat block prints on its Intelligence
// line, from Non- (0) to Godlike. The Dungeon Master Guide's Ability Scores for Monsters
// reads a monster's Intelligence, Wisdom and Charisma off the column for its rating
// (Tables 9 to 11), as it reads Strength, Dexterity and Constitution off its size. Kept
// apart from Creature so the registry and the editor can name a rating without one.
//
// Usage:
//
//   const IntelligenceRating ogre = parse_intelligence_rating("low"); // what monsters.json carries
//   encode_intelligence_rating(ogre); // -> "low", what a save writes back
//   next_intelligence_rating(ogre); // -> IntelligenceRating::AVERAGE, the editor's next value

// The ratings in the Monstrous Manual's order. NON is the book's Non- (0), which Tables
// 9 to 11 print no column for; every other value is one of their columns.
enum class IntelligenceRating
{
	NON,
	ANIMAL,
	SEMI,
	LOW,
	AVERAGE,
	VERY,
	HIGH,
	EXCEPTIONAL,
	GENIUS,
	SUPRA_GENIUS,
	GODLIKE
};

// Every IntelligenceRating, in the order the enum declares them, so a cycle through the
// editor's field reaches all of them and adding one is a single edit here.
inline constexpr std::array<IntelligenceRating, 11> ALL_INTELLIGENCE_RATING = {
	IntelligenceRating::NON,
	IntelligenceRating::ANIMAL,
	IntelligenceRating::SEMI,
	IntelligenceRating::LOW,
	IntelligenceRating::AVERAGE,
	IntelligenceRating::VERY,
	IntelligenceRating::HIGH,
	IntelligenceRating::EXCEPTIONAL,
	IntelligenceRating::GENIUS,
	IntelligenceRating::SUPRA_GENIUS,
	IntelligenceRating::GODLIKE
};

// The next IntelligenceRating in that order, wrapping at the end.
//
// Example:
//   next_intelligence_rating(IntelligenceRating::NON); // -> IntelligenceRating::ANIMAL
//   next_intelligence_rating(IntelligenceRating::GODLIKE); // -> IntelligenceRating::NON
[[nodiscard]] inline IntelligenceRating next_intelligence_rating(IntelligenceRating value)
{
	const auto found = std::ranges::find(ALL_INTELLIGENCE_RATING, value);
	if (found == ALL_INTELLIGENCE_RATING.end() || found + 1 == ALL_INTELLIGENCE_RATING.end())
	{
		return ALL_INTELLIGENCE_RATING.front();
	}
	return *(found + 1);
}

// The name monsters.json carries for a rating.
//
// Example:
//   encode_intelligence_rating(IntelligenceRating::SUPRA_GENIUS); // -> "supra_genius"
[[nodiscard]] inline std::string_view encode_intelligence_rating(IntelligenceRating rating)
{
	switch (rating)
	{

	case IntelligenceRating::NON:
	{
		return "non";
	}

	case IntelligenceRating::ANIMAL:
	{
		return "animal";
	}

	case IntelligenceRating::SEMI:
	{
		return "semi";
	}

	case IntelligenceRating::LOW:
	{
		return "low";
	}

	case IntelligenceRating::AVERAGE:
	{
		return "average";
	}

	case IntelligenceRating::VERY:
	{
		return "very";
	}

	case IntelligenceRating::HIGH:
	{
		return "high";
	}

	case IntelligenceRating::EXCEPTIONAL:
	{
		return "exceptional";
	}

	case IntelligenceRating::GENIUS:
	{
		return "genius";
	}

	case IntelligenceRating::SUPRA_GENIUS:
	{
		return "supra_genius";
	}

	case IntelligenceRating::GODLIKE:
	{
		return "godlike";
	}
	}

	return "average";
}

// The rating a record names. Throws naming what it read.
//
// Example:
//   parse_intelligence_rating("genius"); // -> IntelligenceRating::GENIUS
//   parse_intelligence_rating("brilliant"); // throws std::runtime_error
[[nodiscard]] inline IntelligenceRating parse_intelligence_rating(std::string_view name)
{
	const auto named = [name](IntelligenceRating rating)
	{
		return encode_intelligence_rating(rating) == name;
	};

	// Searched against the encoder's names, so the two cannot disagree.
	const auto found = std::ranges::find_if(ALL_INTELLIGENCE_RATING, named);
	if (found == ALL_INTELLIGENCE_RATING.end())
	{
		throw std::runtime_error(std::format("unknown intelligence rating '{}'", name));
	}
	return *found;
}

// end of file: IntelligenceRating.h
