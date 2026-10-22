// file: PotionDurationTest.cpp
// How long a potion lasts, by the Dungeon Master's Guide, rolled once when it is drunk.
// The book prints potion durations in turns, and "Ten combat rounds equal a turn" (PHB,
// PDF page 184), so every expected figure here is the book's turns times ten, computed
// here rather than read from the game's constant.
//
// - Invisibility, "3-6 turns" (DMG PDF page 891).
// - Giant strength states none, so "four complete turns plus d4 additional turns"
//   (DMG PDF page 887) applies.
// - Fire resistance, "The potion lasts one turn" (DMG PDF page 889).
// - Cold resistance is not in the DMG, and keeps the game's five turns.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=PotionDurationTest.*

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <variant>

#include <nlohmann/json.hpp>

#include "src/ArmorClass.h"
#include "src/BuffSystem.h"
#include "src/ExperienceReward.h"
#include "src/GameContext.h"
#include "src/HealthPool.h"
#include "src/InventoryOperations.h"
#include "src/Item.h"
#include "src/ItemCreator.h"
#include "src/Pickable.h"
#include "src/Player.h"
#include "src/RandomDice.h"
#include "src/TurnSchedule.h"
#include "src/Vector2D.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
// The book's turn, in its rounds: "Ten combat rounds equal a turn" (PHB, PDF page 184).
constexpr int ROUNDS_IN_A_TURN = 10;
} // namespace

class PotionDurationTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		mock.player = std::make_unique<Player>(Vector2D{ 2, 2 });
		mock.player->experienceReward = std::make_unique<ExperienceReward>(0);
		mock.player->healthPool = std::make_unique<HealthPool>(10);
		mock.player->armorClass = std::make_unique<ArmorClass>(10);
	}

	// Drinks the named potion from the pack, with its duration die scripted to `rolled`.
	// Returns the buff it grants, read from the potion, so the test follows the data.
	BuffType drink(const std::string& key, int rolled)
	{
		auto created = ItemCreator::create(key, drinker().position, ctx);
		Item& potion = *created;
		// Placed as starting gear is, past the weight gate a fixture's Strength 0 would shut.
		[[maybe_unused]] const auto added = InventoryOperations::add_item(drinker().inventoryData, std::move(created));
		EXPECT_TRUE(added.has_value()) << "the potion did not fit in the pack";
		const BuffType granted = std::get<Consumable>(*potion.behavior).buffType;

		mock.dice.set_next_roll(rolled);
		use_item(*potion.behavior, potion, drinker(), ctx);
		return granted;
	}

	// Whether the buff is still on the drinker once the clock reads `rounds` after drinking.
	bool lasts_to(BuffType granted, int rounds)
	{
		ctx.buffSystem->update_creature_buffs(drinker(), expiry_time(0, rounds));
		return ctx.buffSystem->has_buff(drinker(), granted);
	}

	Player& drinker()
	{
		return ctx.player_concrete();
	}

	MockGameContext mock{};
	GameContext ctx{ mock.to_game_context() };
};

// The least of "3-6 turns": in force through its last round, over at its end.
TEST_F(PotionDurationTest, InvisibilityRolledLowLastsThreeTurns)
{
	const BuffType granted = drink("invisibility_potion", 1);

	EXPECT_TRUE(lasts_to(granted, 3 * ROUNDS_IN_A_TURN - 1));
	EXPECT_FALSE(lasts_to(granted, 3 * ROUNDS_IN_A_TURN));
}

// The most of "3-6 turns".
TEST_F(PotionDurationTest, InvisibilityRolledHighLastsSixTurns)
{
	const BuffType granted = drink("invisibility_potion", 4);

	EXPECT_TRUE(lasts_to(granted, 6 * ROUNDS_IN_A_TURN - 1)) << "gone before six turns";
	EXPECT_FALSE(lasts_to(granted, 6 * ROUNDS_IN_A_TURN));
}

// "4+d4" turns with the die at its least: five turns.
TEST_F(PotionDurationTest, GiantStrengthRolledLowLastsFiveTurns)
{
	const BuffType granted = drink("potion_of_giant_strength", 1);

	EXPECT_TRUE(lasts_to(granted, 5 * ROUNDS_IN_A_TURN - 1));
	EXPECT_FALSE(lasts_to(granted, 5 * ROUNDS_IN_A_TURN));
}

// "4+d4" turns with the die at its most: eight turns.
TEST_F(PotionDurationTest, GiantStrengthRolledHighLastsEightTurns)
{
	const BuffType granted = drink("potion_of_giant_strength", 4);

	EXPECT_TRUE(lasts_to(granted, 8 * ROUNDS_IN_A_TURN - 1)) << "gone before eight turns";
	EXPECT_FALSE(lasts_to(granted, 8 * ROUNDS_IN_A_TURN));
}

// "The potion lasts one turn", whatever the dice would have said.
TEST_F(PotionDurationTest, FireResistanceLastsOneTurn)
{
	const BuffType granted = drink("potion_of_fire_resistance", 4);

	EXPECT_TRUE(lasts_to(granted, ROUNDS_IN_A_TURN - 1));
	EXPECT_FALSE(lasts_to(granted, ROUNDS_IN_A_TURN)) << "still resisting fire after the book's one turn";
}

// Not in the book, so the game's own five turns stand.
TEST_F(PotionDurationTest, ColdResistanceKeepsItsFiveTurns)
{
	const BuffType granted = drink("potion_of_cold_resistance", 4);

	EXPECT_TRUE(lasts_to(granted, 5 * ROUNDS_IN_A_TURN - 1));
	EXPECT_FALSE(lasts_to(granted, 5 * ROUNDS_IN_A_TURN));
}

// A potion carried across a save keeps the dice it was authored with, not a roll.
TEST_F(PotionDurationTest, ASavedPotionKeepsItsDice)
{
	auto potion = ItemCreator::create("invisibility_potion", drinker().position, ctx);
	nlohmann::json saved;
	save_behavior(*potion->behavior, saved);
	ASSERT_EQ(saved["duration"], "1d4+2");
	ASSERT_EQ(saved["durationUnit"], "turns");

	nlohmann::json resaved;
	save_behavior(load_behavior(saved), resaved);

	EXPECT_EQ(resaved["duration"], "1d4+2");
	EXPECT_EQ(resaved["durationUnit"], "turns");
}

// end of file: PotionDurationTest.cpp
