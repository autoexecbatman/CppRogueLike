// file: CreatureRecordSaveTest.cpp
// A creature keeps across a save what its monster record gave it: its attitude, whether
// the player may swap places with it, its morale, its corpse's weight, its body plan, and
// what it holds in each slot. Each test sets a value the member does not start with,
// because a field the save dropped reads back as the member's default, and a default
// cannot show that it was lost.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=CreatureRecordSaveTest.*

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <memory>
#include <string_view>

#include "src/ArmorClass.h"
#include "src/Attitude.h"
#include "src/Colors.h"
#include "src/Creature.h"
#include "src/EquipmentSlot.h"
#include "src/ExperienceReward.h"
#include "src/GameContext.h"
#include "src/HealthPool.h"
#include "src/Item.h"
#include "src/MonsterCreator.h"
#include "src/MonsterRegistry.h"
#include "src/Player.h"
#include "src/PlayerAttacker.h"
#include "src/RandomDice.h"
#include "src/Vector2D.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
// More rolls than building a shopkeeper and stocking its shop makes.
constexpr int SCRIPTED_ROLLS = 400;
} // namespace

class CreatureRecordSaveTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();
	}

	// A creature with nothing set, as a load starts from.
	static std::unique_ptr<Creature> blank()
	{
		return std::make_unique<Creature>(Vector2D{ 0, 0 }, ActorData{ TileRef{}, "temp", ColorPairId::WHITE_BLACK });
	}

	// What a fresh creature reads back from the save the given one writes.
	static std::unique_ptr<Creature> saved_and_loaded(Creature& original)
	{
		nlohmann::json record;
		original.save(record);
		std::unique_ptr<Creature> loaded = blank();
		loaded->load(record);
		return loaded;
	}

	// A monster built from its record, as the spawn table builds one.
	std::unique_ptr<Creature> build(std::string_view key)
	{
		for (int roll = 0; roll < SCRIPTED_ROLLS; ++roll)
		{
			mock.dice.set_next_roll(RandomDice::HIGHEST);
		}
		return MonsterCreator::create_from_params(Vector2D{ 0, 0 }, mock.monsterRegistry.get_params(key), ctx);
	}

	MockGameContext mock{};
	GameContext ctx{};
};

// The member starts HOSTILE.
TEST_F(CreatureRecordSaveTest, AnAttitudeSurvivesASave)
{
	std::unique_ptr<Creature> original = blank();
	original->set_attitude(Attitude::PEACEFUL);

	EXPECT_EQ(saved_and_loaded(*original)->get_attitude(), Attitude::PEACEFUL);
}

// The member starts true.
TEST_F(CreatureRecordSaveTest, StandingItsGroundSurvivesASave)
{
	std::unique_ptr<Creature> original = blank();
	original->set_displaceable(false);

	EXPECT_FALSE(saved_and_loaded(*original)->is_displaceable());
}

// The member starts at 10.
TEST_F(CreatureRecordSaveTest, MoraleSurvivesASave)
{
	std::unique_ptr<Creature> original = blank();
	original->set_morale(16);

	EXPECT_EQ(saved_and_loaded(*original)->get_morale(), 16);
}

// The member starts at 50.
TEST_F(CreatureRecordSaveTest, ACorpseWeightSurvivesASave)
{
	std::unique_ptr<Creature> original = blank();
	original->set_corpse_weight(170);

	EXPECT_EQ(saved_and_loaded(*original)->get_corpse_weight(), 170);
}

// The member starts with no slots. hands_only has both hands and nothing else.
TEST_F(CreatureRecordSaveTest, ABodyPlanSurvivesASave)
{
	std::unique_ptr<Creature> original = blank();
	original->set_body_plan(mock.body_plans.get("hands_only"));

	const std::unique_ptr<Creature> loaded = saved_and_loaded(*original);

	EXPECT_TRUE(loaded->has_slot(EquipmentSlot::RIGHT_HAND));
	EXPECT_TRUE(loaded->has_slot(EquipmentSlot::LEFT_HAND));
	EXPECT_FALSE(loaded->has_slot(EquipmentSlot::BODY)) << "a hands-only creature came back with a body slot";
}

// The goblin's record arms it with a short sword in its right hand.
TEST_F(CreatureRecordSaveTest, AMonstersWeaponSurvivesASave)
{
	std::unique_ptr<Creature> goblin = build("goblin");
	ASSERT_NE(goblin->get_equipped_item(EquipmentSlot::RIGHT_HAND), nullptr) << "the goblin was built unarmed";

	const std::unique_ptr<Creature> loaded = saved_and_loaded(*goblin);

	const Item* weapon = loaded->get_equipped_item(EquipmentSlot::RIGHT_HAND);
	ASSERT_NE(weapon, nullptr) << "the loaded goblin has nothing in its right hand";
	EXPECT_EQ(weapon->itemKey, "short_sword");
}

// The shopkeeper as data builds it, through a save: peaceful, holding its ground, its
// corpse its record's weight, a humanoid with its dagger.
TEST_F(CreatureRecordSaveTest, AShopkeeperIsStillItselfAfterASave)
{
	std::unique_ptr<Creature> shopkeeper = build("shopkeeper");

	const std::unique_ptr<Creature> loaded = saved_and_loaded(*shopkeeper);

	EXPECT_EQ(loaded->get_attitude(), Attitude::PEACEFUL) << "the loaded shopkeeper turned hostile";
	EXPECT_FALSE(loaded->is_displaceable()) << "the loaded shopkeeper can be shoved aside";
	EXPECT_EQ(loaded->get_corpse_weight(), mock.monsterRegistry.get_params("shopkeeper").corpseWeight);
	EXPECT_TRUE(loaded->has_slot(EquipmentSlot::BODY)) << "the loaded shopkeeper has no humanoid slots";
	const Item* dagger = loaded->get_equipped_item(EquipmentSlot::RIGHT_HAND);
	ASSERT_NE(dagger, nullptr) << "the loaded shopkeeper lost its dagger";
	EXPECT_EQ(dagger->itemKey, "dagger");
}

// The mimic's record gives it Champion morale, 16.
TEST_F(CreatureRecordSaveTest, AMimicsMoraleIsStillItsRecordsAfterASave)
{
	std::unique_ptr<Creature> mimic = build("mimic");
	ASSERT_EQ(mimic->get_morale(), 16);

	EXPECT_EQ(saved_and_loaded(*mimic)->get_morale(), 16);
}

// The player is saved through the same members. hands_only differs from the humanoid plan
// a new game gives the player, so a loaded player that reads humanoid lost the save's plan.
TEST_F(CreatureRecordSaveTest, APlayersBodyPlanIsTheOneItsSaveHeld)
{
	Player original{ Vector2D{ 0, 0 } };
	// The components a player load restores and then asserts.
	original.experienceReward = std::make_unique<ExperienceReward>(0);
	original.armorClass = std::make_unique<ArmorClass>(10);
	original.healthPool = std::make_unique<HealthPool>(30);
	original.attacker = std::make_unique<PlayerAttacker>(original);
	original.set_body_plan(mock.body_plans.get("hands_only"));
	nlohmann::json record;
	original.save(record);

	Player loaded{ Vector2D{ 0, 0 } };
	loaded.load(record);

	EXPECT_TRUE(loaded.has_slot(EquipmentSlot::RIGHT_HAND));
	EXPECT_FALSE(loaded.has_slot(EquipmentSlot::BODY)) << "the loaded player has a body slot its save did not hold";
}

// end of file: CreatureRecordSaveTest.cpp
