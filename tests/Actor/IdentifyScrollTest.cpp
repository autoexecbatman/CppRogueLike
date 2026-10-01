// file: IdentifyScrollTest.cpp
//
// What reading an identify scroll does to the pack it is read over.
//
// The scroll identifies everything the wearer is carrying that is not already fully
// identified, says how many that was, and is consumed either way - reading it is the
// cost, so a pack that was already known still burns it. That last part is the branch
// nothing covered before 2026-10-01: the loop was exercised by play and the
// already-identified case by nothing.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=IdentifyScrollTest.*

#include <gtest/gtest.h>

#include <memory>
#include <string_view>

#include <string>

#include "src/Creature.h"
#include "src/InventoryOperations.h"
#include "src/Item.h"
#include "src/ItemCreator.h"
#include "src/LogMessage.h"
#include "src/MessageSystem.h"
#include "src/Pickable.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
constexpr Vector2D ON_THE_FLOOR{ 2, 2 };
} // namespace

class IdentifyScrollTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();

		reader = std::make_unique<Creature>(
			ON_THE_FLOOR, ActorData{ TileRef{}, "reader", ColorPairId::WHITE_BLACK });
		// A creature built in a test has no Strength, and the pack is weight-gated by
		// it, so without this nothing can be carried.
		reader->set_strength(18);

		// The scroll has to be in the pack: use() ends by consuming it, and consuming
		// means removing it from the wearer's inventory. A scroll held nowhere cannot
		// be spent, which is what the return value reports.
		scrollItem = &carried("identify_scroll");
	}

	// Puts an item in the reader's pack and hands it back, so a test can ask about it
	// after the scroll has run.
	Item& carried(std::string_view key)
	{
		auto item = ItemCreator::create(key, ON_THE_FLOOR, ctx);
		Item* held = item.get();
		[[maybe_unused]] const auto added = InventoryOperations::add_item(reader->inventoryData, std::move(item));
		return *held;
	}

	bool read_the_scroll()
	{
		IdentifyScroll behavior{};
		return use(behavior, *scrollItem, *reader, ctx);
	}

	// Whether any stored message holds the given text. The system keeps each message as
	// a vector of coloured runs, so the words have to be joined before searching.
	bool said_anything_about(std::string_view wanted) const
	{
		for (size_t index = 0; index < ctx.messageSystem->get_stored_message_count(); ++index)
		{
			std::string whole;
			for (const LogMessage& run : ctx.messageSystem->get_attack_message_at(index))
			{
				whole += run.text;
			}
			if (whole.find(wanted) != std::string::npos)
			{
				return true;
			}
		}
		return false;
	}

	MockGameContext mock{};
	GameContext ctx{};
	std::unique_ptr<Creature> reader;
	Item* scrollItem{ nullptr };
};

// The thing the scroll is for.
TEST_F(IdentifyScrollTest, AnUnknownItemBecomesKnown)
{
	Item& unknown = carried("health_potion");
	ASSERT_FALSE(unknown.is_fully_identified()) << "the fixture handed out an item that was already identified";

	read_the_scroll();

	EXPECT_TRUE(unknown.is_fully_identified()) << "the scroll was read and the potion is still unknown";
}

// Every unknown item, not the first one it meets.
TEST_F(IdentifyScrollTest, EveryUnknownItemInThePackBecomesKnown)
{
	Item& first = carried("health_potion");
	Item& second = carried("mana_potion");

	read_the_scroll();

	EXPECT_TRUE(first.is_fully_identified());
	EXPECT_TRUE(second.is_fully_identified()) << "the scroll stopped before the end of the pack";
}

// The branch nothing covered: reading it over a pack that is already known. The scroll
// is still spent, because reading it is the cost.
TEST_F(IdentifyScrollTest, APackAlreadyKnownStillSpendsTheScroll)
{
	Item& known = carried("health_potion");
	known.identify_all();
	// The scroll is in the pack too and counts as an item, so it has to be identified
	// as well or the count comes out at one and this never reaches the branch it names.
	scrollItem->identify_all();

	const bool spent = read_the_scroll();

	EXPECT_TRUE(spent) << "a scroll read over an already-identified pack was not consumed";
	EXPECT_TRUE(said_anything_about("already identified"))
		<< "the pack was fully identified and the scroll did not say so";
}

// A pack holding nothing but the scroll is a legal thing to read it over.
TEST_F(IdentifyScrollTest, APackHoldingOnlyTheScrollStillSpendsIt)
{
	ASSERT_EQ(reader->inventoryData.items.size(), 1u) << "the fixture put something else in the pack";

	EXPECT_TRUE(read_the_scroll()) << "a scroll read over a pack holding only itself was not consumed";
}
