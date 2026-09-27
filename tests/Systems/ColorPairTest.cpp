// file: ColorPairTest.cpp
// The colour pairs as a closed set: the table the renderer fills has a slot for
// every one of them, and every colour the shipped data names is one of them.
//
// What this is for. A colour pair used to be a bare integer - a constant in a
// header, a hand-kept table size in another, an index written out longhand in the
// renderer's initialiser, and a raw number in items.json. Nothing tied the four
// together, so the size could fall behind the list and a record could carry a
// number naming no pair at all. The enum closes the set; these are the two claims
// that cannot be made at compile time.
//
// The round trip and the editor's cycle are covered by CodecRoundTripTest and
// EnumCycleTest, which every enum with a string form goes through.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=ColorPairTest.*

#include <gtest/gtest.h>

#include <fstream>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

#include "src/Colors.h"
#include "src/Paths.h"

// The renderer indexes its table by the pair itself, so every pair needs a slot and
// the size has to be derived from the list rather than written down beside it.
TEST(ColorPairTest, TheTableHasASlotForEveryPair)
{
	for (const ColorPairId pair : ALL_COLOR_PAIR)
	{
		EXPECT_LT(color_pair_index(pair), COLOR_PAIR_TABLE_SIZE) << color_pair_name(pair);
	}

	// Slot zero belongs to no pair, which is the one place the offset shows.
	EXPECT_EQ(color_pair_index(ALL_COLOR_PAIR.front()), 1u);
	EXPECT_EQ(COLOR_PAIR_TABLE_SIZE, ALL_COLOR_PAIR.size() + 1);
}

// Every record the game ships names a pair that exists. The loaders throw on one
// that does not, so this is what says the shipped data is loadable at all - the
// integers these fields used to hold could name anything.
TEST(ColorPairTest, EveryColourTheDataShipsNamesAPair)
{
	for (const std::string_view file : { Paths::ITEMS, Paths::MONSTERS })
	{
		std::ifstream stream(Paths::resolve(file));
		ASSERT_TRUE(stream.is_open()) << file << " did not open";

		nlohmann::json records;
		stream >> records;

		int checked = 0;
		for (const auto& [key, record] : records.items())
		{
			if (!record.contains("color"))
			{
				continue;
			}
			const std::string named = record.at("color").get<std::string>();
			EXPECT_NO_THROW((void)parse_color_pair(named)) << key << " carries '" << named << "'";
			++checked;
		}

		EXPECT_GT(checked, 0) << file << " carried no colours at all, so this checked nothing";
	}
}

// end of file: ColorPairTest.cpp
