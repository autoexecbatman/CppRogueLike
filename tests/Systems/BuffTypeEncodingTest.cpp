// file: BuffTypeEncodingTest.cpp
// Pins the string every BuffType encodes to. These strings are the on-disk
// format in items.json, so changing one silently reinterprets authored content.
//
// The list here is written out deliberately rather than derived from the
// encoder: a test that reads its expectations from the code under test cannot
// disagree with it. If a BuffType is added and this file is not updated, the
// count assertion below fails.

#include <gtest/gtest.h>

#include <array>
#include <string_view>

#include "src/Systems/BuffType.h"

namespace
{
struct EncodingCase
{
	BuffType type{ BuffType::NONE };
	std::string_view expected{};
};

// Every BuffType and the string it must produce.
constexpr std::array<EncodingCase, 21> ENCODINGS = { {
	{ BuffType::NONE, "none" },
	{ BuffType::INVISIBILITY, "invisibility" },
	{ BuffType::BLESS, "bless" },
	{ BuffType::SHIELD, "shield" },
	{ BuffType::STRENGTH, "strength" },
	{ BuffType::DEXTERITY, "dexterity" },
	{ BuffType::CONSTITUTION, "constitution" },
	{ BuffType::INTELLIGENCE, "intelligence" },
	{ BuffType::WISDOM, "wisdom" },
	{ BuffType::CHARISMA, "charisma" },
	{ BuffType::SPEED, "speed" },
	{ BuffType::FIRE_RESISTANCE, "fire_resistance" },
	{ BuffType::COLD_RESISTANCE, "cold_resistance" },
	{ BuffType::LIGHTNING_RESISTANCE, "lightning_resistance" },
	{ BuffType::POISON_RESISTANCE, "poison_resistance" },
	{ BuffType::SLEEP, "sleep" },
	{ BuffType::HOLD_PERSON, "hold_person" },
	{ BuffType::SANCTUARY, "sanctuary" },
	{ BuffType::PROTECTION_FROM_EVIL, "protection_from_evil" },
	{ BuffType::SILENCE, "silence" },
	{ BuffType::WEBBED, "webbed" },
} };
} // namespace

// Each buff type encodes to its documented string.
TEST(BuffTypeEncodingTest, EveryTypeEncodesToItsDocumentedString)
{
	for (const auto& encoding : ENCODINGS)
	{
		EXPECT_EQ(encode_buff_type(encoding.type), encoding.expected);
	}
}

// No two buff types share a string, or decoding could not tell them apart.
TEST(BuffTypeEncodingTest, EncodingsAreUnique)
{
	for (size_t first = 0; first < ENCODINGS.size(); ++first)
	{
		for (size_t second = first + 1; second < ENCODINGS.size(); ++second)
		{
			EXPECT_NE(ENCODINGS[first].expected, ENCODINGS[second].expected)
				<< "duplicate at " << first << " and " << second;
		}
	}
}

// A buff type that falls through the encoder returns "none", which is
// indistinguishable from BuffType::NONE on disk. Only NONE may encode to it.
TEST(BuffTypeEncodingTest, OnlyNoneEncodesToNone)
{
	for (const auto& encoding : ENCODINGS)
	{
		if (encoding.type == BuffType::NONE)
		{
			continue;
		}

		EXPECT_NE(encode_buff_type(encoding.type), "none")
			<< "an unhandled buff type is silently encoding as none";
	}
}
