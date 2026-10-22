// file: ActorLoadTest.cpp
// Actor::load refuses a record missing any key Actor::save always writes, by throwing.
//
// What it is for. Actor::save writes "position", "direction", "actorData" and "states"
// for every creature, item and trap. Reading them through a const json's operator[]
// aborted in Debug when one was missing, and read past the end of the object in Release.
// A throw is what load_all turns into a refused save (issues/0322). An actor with no
// states saves "states" as null, and that record still loads.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=ActorLoad*:EveryKeyActorSaveWrites*

#include <gtest/gtest.h>

#include <format>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

#include "src/Actor.h"
#include "src/GameContext.h"
#include "src/Item.h"
#include "src/ItemCreator.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
// Away from the origin and with x and y unequal, so a default or a swap cannot pass.
constexpr Vector2D SAVED_POSITION{ 7, 3 };
constexpr Vector2D SAVED_DIRECTION{ 1, -1 };
} // namespace

class ActorLoadTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();
	}

	// An item from the item data, saved through its own writer.
	nlohmann::json saved_item(bool withState)
	{
		auto item = ItemCreator::create("dagger", SAVED_POSITION, ctx);
		item->direction = SAVED_DIRECTION;
		if (withState)
		{
			item->add_state(ActorState::BLOCKS);
		}
		nlohmann::json record;
		item->save(record);
		return record;
	}

	// A fresh item for a record to be loaded into.
	std::unique_ptr<Item> blank_item()
	{
		return ItemCreator::create("dagger", Vector2D{ 0, 0 }, ctx);
	}

	MockGameContext mock{};
	GameContext ctx{};
};

TEST_F(ActorLoadTest, ARoundTripKeepsPositionDirectionAndStates)
{
	const nlohmann::json record = saved_item(true);
	auto loaded = blank_item();

	loaded->load(record);

	EXPECT_EQ(loaded->position, SAVED_POSITION);
	EXPECT_EQ(loaded->direction, SAVED_DIRECTION);
	EXPECT_TRUE(loaded->has_state(ActorState::BLOCKS)) << "the saved state was lost";
}

TEST_F(ActorLoadTest, AnActorWithNoStatesRoundTripsWithNone)
{
	const nlohmann::json record = saved_item(false);
	ASSERT_TRUE(record.at("states").is_null()) << "the writer no longer saves an empty state list as null, so this pin pins nothing";
	auto loaded = blank_item();
	loaded->add_state(ActorState::BLOCKS);

	loaded->load(record);

	EXPECT_TRUE(loaded->states.empty()) << "an actor saved with no states loaded with some";
}

TEST_F(ActorLoadTest, APositionWithoutItsXIsRefused)
{
	nlohmann::json record = saved_item(true);
	record.at("position").erase("x");
	auto loaded = blank_item();

	EXPECT_THROW(loaded->load(record), std::exception) << "a position without x loaded";
}

TEST_F(ActorLoadTest, ADirectionWithoutItsYIsRefused)
{
	nlohmann::json record = saved_item(true);
	record.at("direction").erase("y");
	auto loaded = blank_item();

	EXPECT_THROW(loaded->load(record), std::exception) << "a direction without y loaded";
}

// The same item, once per key Actor::save always writes.
class ActorLoadKeyTest : public ActorLoadTest, public ::testing::WithParamInterface<std::string_view>
{
};

TEST_P(ActorLoadKeyTest, ARecordMissingTheKeyIsRefused)
{
	const std::string key{ GetParam() };
	nlohmann::json record = saved_item(true);
	ASSERT_TRUE(record.contains(key)) << std::format("the writer did not write \"{}\", so erasing it proves nothing", key);
	record.erase(key);
	auto loaded = blank_item();

	EXPECT_THROW(loaded->load(record), std::exception) << "a record without \"" << key << "\" loaded";
}

// Each case is named for its key, so a failure says which one without reading the output.
const auto name_for_actor_key = [](const ::testing::TestParamInfo<std::string_view>& info)
{
	return std::string{ info.param };
};

INSTANTIATE_TEST_SUITE_P(
	EveryKeyActorSaveWrites,
	ActorLoadKeyTest,
	::testing::Values("position", "direction", "actorData", "states"),
	name_for_actor_key);
