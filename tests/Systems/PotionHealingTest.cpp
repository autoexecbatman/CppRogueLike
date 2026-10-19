// file: PotionHealingTest.cpp
// How much a healing potion heals, by the Dungeon Master's Guide, rolled once when it
// is drunk:
//
// - Healing: "the potion restores 2d4 + 2 hit points of damage" (DMG PDF page 890).
// - Extra-healing: "This potion restores 3d8 + 3 hit points of damage when wholly
//   consumed" (DMG PDF page 889). The game drinks a potion whole, so the book's thirds
//   are not drunk.
//
// The dice are scripted to their faces and each expected figure is the book's expression
// at those faces, worked here. The drinker has room for every heal, so none is clamped.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=PotionHealingTest.*

#include <gtest/gtest.h>

#include <initializer_list>
#include <memory>
#include <string>

#include <nlohmann/json.hpp>

#include "src/ArmorClass.h"
#include "src/ExperienceReward.h"
#include "src/GameContext.h"
#include "src/HealthPool.h"
#include "src/InventoryOperations.h"
#include "src/Item.h"
#include "src/ItemCreator.h"
#include "src/Pickable.h"
#include "src/Player.h"
#include "src/RandomDice.h"
#include "src/Vector2D.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
// More than extra-healing's most, 27, so no heal is cut short by the maximum.
constexpr int DRINKER_MAX_HP = 40;
} // namespace

class PotionHealingTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		mock.player = std::make_unique<Player>(Vector2D{ 2, 2 });
		mock.player->experienceReward = std::make_unique<ExperienceReward>(0);
		mock.player->healthPool = std::make_unique<HealthPool>(DRINKER_MAX_HP);
		mock.player->armorClass = std::make_unique<ArmorClass>(10);
	}

	// Drinks the named potion from the pack with its dice scripted to `faces`, in order.
	void drink(const std::string& key, std::initializer_list<int> faces)
	{
		auto created = ItemCreator::create(key, drinker().position, ctx);
		Item& potion = *created;
		// Placed as starting gear is, past the weight gate a fixture's Strength 0 would shut.
		[[maybe_unused]] const auto added = InventoryOperations::add_item(drinker().inventoryData, std::move(created));
		EXPECT_TRUE(added.has_value()) << "the potion did not fit in the pack";

		for (const int face : faces)
		{
			mock.dice.set_next_roll(face);
		}
		use_item(*potion.behavior, potion, drinker(), ctx);
	}

	// Hit points the named potion restores to a drinker at 1, with its dice at `faces`.
	int heals(const std::string& key, std::initializer_list<int> faces)
	{
		drinker().set_hp(1);
		drink(key, faces);
		return drinker().get_hp() - 1;
	}

	Player& drinker()
	{
		return ctx.player_concrete();
	}

	MockGameContext mock{};
	GameContext ctx{ mock.to_game_context() };
};

// 2d4 + 2 with both dice at 1.
TEST_F(PotionHealingTest, HealingRolledLowRestoresFour)
{
	EXPECT_EQ(heals("health_potion", { 1, 1 }), 1 + 1 + 2);
}

// 2d4 + 2 with both dice at 4.
TEST_F(PotionHealingTest, HealingRolledHighRestoresTen)
{
	EXPECT_EQ(heals("health_potion", { 4, 4 }), 4 + 4 + 2);
}

// 3d8 + 3 with every die at 1.
TEST_F(PotionHealingTest, ExtraHealingRolledLowRestoresSix)
{
	EXPECT_EQ(heals("potion_of_extra_healing", { 1, 1, 1 }), 1 + 1 + 1 + 3);
}

// 3d8 + 3 with every die at 8: the book's most, which the game's old 30 exceeded.
TEST_F(PotionHealingTest, ExtraHealingRolledHighRestoresTwentySeven)
{
	EXPECT_EQ(heals("potion_of_extra_healing", { 8, 8, 8 }), 8 + 8 + 8 + 3);
}

// A potion carried across a save keeps the dice it was authored with, not a roll.
TEST_F(PotionHealingTest, ASavedHealingPotionKeepsItsDice)
{
	auto potion = ItemCreator::create("health_potion", drinker().position, ctx);
	nlohmann::json saved;
	save_behavior(*potion->behavior, saved);
	ASSERT_EQ(saved["amount"], "2d4+2");

	nlohmann::json resaved;
	save_behavior(load_behavior(saved), resaved);

	EXPECT_EQ(resaved["amount"], "2d4+2");
}

// A fixed amount is still fixed: giant strength sets the score it always set.
TEST_F(PotionHealingTest, GiantStrengthStillSetsItsScore)
{
	// The one die is its duration's.
	drink("potion_of_giant_strength", { 1 });

	EXPECT_EQ(drinker().get_strength(), 19);
}

// end of file: PotionHealingTest.cpp
