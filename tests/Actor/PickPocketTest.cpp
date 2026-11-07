// file: PickPocketTest.cpp
// Filching from a creature beside the thief, by the Player's Handbook's Pick
// Pockets (PDF page 86).
//
// The book's rule: a failed attempt "means the thief did not get an item, but
// it does not mean that his attempt was detected... subtract three times the
// victim's level from 100. If the thief's pick pockets roll was equal to or
// greater than this number, the attempt is detected... the attempt may succeed
// and be noticed at the same time."
//
// The thief is a human rogue of Dexterity 13 wearing nothing: Pick Pockets base
// 15 and Table 29's +5 unarmoured, so his score is 20 plus what he bought.
// Every d100 is scripted. A roll left unscripted is a real one, so a case that
// expects no roll queues a certain success and checks nothing moved.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=PickPocketTest.*

#include <gtest/gtest.h>

#include <memory>
#include <string_view>
#include <utility>
#include <vector>

#include "src/Actor.h"
#include "src/ArmorClass.h"
#include "src/Attitude.h"
#include "src/Colors.h"
#include "src/Creature.h"
#include "src/CreatureClass.h"
#include "src/EquipmentSlot.h"
#include "src/ExperienceReward.h"
#include "src/GameContext.h"
#include "src/HealthPool.h"
#include "src/InputSystem.h"
#include "src/InventoryOperations.h"
#include "src/Item.h"
#include "src/ItemCreator.h"
#include "src/Map.h"
#include "src/PickPocket.h"
#include "src/Player.h"
#include "src/PlayerAttacker.h"
#include "src/PlayerRaceState.h"
#include "src/Renderer.h"
#include "src/ShopKeeper.h"
#include "src/ThiefSkills.h"
#include "src/TileRef.h"
#include "src/TileType.h"
#include "src/TurnSchedule.h"
#include "src/Vector2D.h"
#include "tests/mocks/MockGameContext.h"

namespace CppRL
{

	class PickPocketTest : public ::testing::Test
	{
	protected:
		void SetUp() override
		{
			ctx = mock.to_game_context();
			ctx.playerOwner = &thief;
			ctx.map = &map;
			ctx.creatures = &creatures;

			map.init_tiles();
			for (int col = 1; col < 19; ++col)
			{
				map.set_tile(Vector2D{ col, 5 }, TileType::FLOOR);
			}

			thief->position = Vector2D{ 5, 5 };
			thief->healthPool = std::make_unique<HealthPool>(20);
			thief->armorClass = std::make_unique<ArmorClass>(10);
			thief->set_creature_class(CreatureClass::ROGUE);
			thief->lifespan->race = PlayerRaceState::HUMAN;
			// Dexterity 13 is the middle of Table 28's flat row, so nothing
			// is folded into the score but the base, the armour and the points.
			thief->set_dexterity(13);
			// A test creature's Strength starts at zero, which carries nothing.
			thief->set_strength(16);
			thief->attacker = std::make_unique<PlayerAttacker>(*thief);
			thief->experienceReward = std::make_unique<ExperienceReward>(0);
		}

		void buy_pick_pockets(int points)
		{
			thief->thiefSkillPoints.at(
				thief_skill_index(ThiefSkill::PICK_POCKETS)) = points;
		}

		// A creature at `where` of the given level, carrying nothing yet.
		Creature& add_creature_at(Vector2D where, int level)
		{
			auto creature = std::make_unique<Creature>(
				where,
				ActorData{ TileRef{}, "merchant", ColorPairId::WHITE_BLACK });
			creature->healthPool = std::make_unique<HealthPool>(8);
			creature->set_creature_level(level);
			creature->set_attitude(Attitude::PEACEFUL);
			Creature& placed = *creature;
			creatures.push_back(std::move(creature));
			return placed;
		}

		// A creature beside the thief carrying one dagger in its pack.
		Creature& add_victim_with_dagger(int level)
		{
			Creature& victim = add_creature_at(Vector2D{ 6, 5 }, level);
			give(victim, "dagger");
			return victim;
		}

		void give(Creature& holder, std::string_view key)
		{
			[[maybe_unused]] const auto added = InventoryOperations::add_item(
				holder.inventoryData,
				ItemCreator::create(key, holder.position, ctx));
			ASSERT_TRUE(added.has_value());
		}

		[[nodiscard]] bool thief_holds(std::string_view name) const
		{
			for (const auto& item : thief->inventoryData.items)
			{
				if (item->get_name() == name)
				{
					return true;
				}
			}
			return false;
		}

		[[nodiscard]] PickPocketAttempt try_with_roll(
			Creature& victim,
			int roll)
		{
			mock.dice.set_next_roll(roll);
			return attempt_pick_pocket(*thief, victim, ctx);
		}

		MockGameContext mock{};
		GameContext ctx{};
		Map map{ 20, 12 };
		std::vector<std::unique_ptr<Creature>> creatures{};
		Renderer renderer{};
		InputSystem input{};
		std::unique_ptr<Player> thief{ std::make_unique<Player>(
			Vector2D{ 5, 5 }) };
	};

	// 100 less three times the level, and the book's own two examples: a
	// 0th-level victim notices only on 00, a 13th-level one on 61 or more.
	TEST_F(PickPocketTest, TheNoticeRollIsAHundredLessThreeTimesTheLevel)
	{
		EXPECT_EQ(pick_pocket_notice_roll(0), 100);
		EXPECT_EQ(pick_pocket_notice_roll(1), 97);
		EXPECT_EQ(pick_pocket_notice_roll(5), 85);
		EXPECT_EQ(pick_pocket_notice_roll(13), 61);
	}

	// 20 and 30 bought is 50: a 50 takes the dagger and a 51 does not.
	TEST_F(PickPocketTest, ARollAtTheScoreTakesTheItem)
	{
		buy_pick_pockets(30);
		ASSERT_EQ(thief->thief_skill(ThiefSkill::PICK_POCKETS), 50);
		Creature& victim = add_victim_with_dagger(1);

		EXPECT_EQ(try_with_roll(victim, 50), PickPocketAttempt::TOOK_ITEM);
		EXPECT_TRUE(thief_holds("dagger"))
			<< "the dagger did not reach the thief";
		EXPECT_TRUE(victim.inventoryData.items.empty())
			<< "the victim still has what was taken";
	}

	TEST_F(PickPocketTest, ARollOneAboveTheScoreTakesNothing)
	{
		buy_pick_pockets(30);
		Creature& victim = add_victim_with_dagger(1);

		EXPECT_EQ(try_with_roll(victim, 51), PickPocketAttempt::TOOK_NOTHING);
		EXPECT_FALSE(thief_holds("dagger"));
		EXPECT_EQ(victim.inventoryData.items.size(), 1u);
	}

	// A failed try is not a detected one: 96 misses a score of 50 and is one
	// short of a level 1 victim's 97.
	TEST_F(PickPocketTest, AFailureBelowTheNoticeRollGoesUnnoticed)
	{
		buy_pick_pockets(30);
		Creature& victim = add_victim_with_dagger(1);

		EXPECT_EQ(try_with_roll(victim, 96), PickPocketAttempt::TOOK_NOTHING);
		EXPECT_EQ(victim.get_attitude(), Attitude::PEACEFUL);
	}

	TEST_F(PickPocketTest, ARollAtTheNoticeRollTurnsTheVictimHostile)
	{
		buy_pick_pockets(30);
		Creature& victim = add_victim_with_dagger(1);

		EXPECT_EQ(try_with_roll(victim, 97), PickPocketAttempt::TOOK_NOTHING);
		EXPECT_EQ(victim.get_attitude(), Attitude::HOSTILE);
	}

	// The victim's level sets the line: a level 5 notices an 85 and not an 84.
	TEST_F(PickPocketTest, AHigherLevelVictimNoticesALowerRoll)
	{
		buy_pick_pockets(30);
		Creature& unaware = add_victim_with_dagger(5);
		EXPECT_EQ(try_with_roll(unaware, 84), PickPocketAttempt::TOOK_NOTHING);
		EXPECT_EQ(unaware.get_attitude(), Attitude::PEACEFUL);

		EXPECT_EQ(try_with_roll(unaware, 85), PickPocketAttempt::TOOK_NOTHING);
		EXPECT_EQ(unaware.get_attitude(), Attitude::HOSTILE);
	}

	// Score 80 against a level 9 victim's 73: a 75 is both.
	TEST_F(PickPocketTest, ATryCanTakeTheItemAndBeNoticedAtOnce)
	{
		buy_pick_pockets(60);
		ASSERT_EQ(thief->thief_skill(ThiefSkill::PICK_POCKETS), 80);
		Creature& victim = add_victim_with_dagger(9);

		EXPECT_EQ(try_with_roll(victim, 75), PickPocketAttempt::TOOK_ITEM);
		EXPECT_TRUE(thief_holds("dagger"));
		EXPECT_EQ(victim.get_attitude(), Attitude::HOSTILE);
	}

	// A shopkeeper's goods are its shelves.
	TEST_F(PickPocketTest, AShopkeeperIsRobbedOfItsStock)
	{
		buy_pick_pockets(30);
		Creature& keeper = add_creature_at(Vector2D{ 6, 5 }, 1);
		keeper.shop = std::make_unique<ShopKeeper>();
		[[maybe_unused]] const auto stocked = InventoryOperations::add_item(
			keeper.shop->get_shop_inventory(),
			ItemCreator::create("dagger", keeper.position, ctx));
		ASSERT_TRUE(stocked.has_value());

		EXPECT_EQ(try_with_roll(keeper, 10), PickPocketAttempt::TOOK_ITEM);
		EXPECT_TRUE(thief_holds("dagger"));
		EXPECT_TRUE(keeper.shop->get_shop_inventory().items.empty());
	}

	// The skill is the thief's; nobody else rolls, and nothing moves.
	TEST_F(PickPocketTest, AClassWithNoScoreCannotTry)
	{
		thief->set_creature_class(CreatureClass::FIGHTER);
		Creature& victim = add_victim_with_dagger(1);

		EXPECT_EQ(try_with_roll(victim, 1), PickPocketAttempt::NOT_A_THIEF);
		EXPECT_FALSE(thief_holds("dagger"));
	}

	// Base 15, Dexterity 9 -15 and padded armour -20 leave nothing, which the
	// book treats as a skill not yet bought up to use.
	TEST_F(PickPocketTest, ARogueWhoseScoreIsNothingCannotTry)
	{
		thief->set_dexterity(9);
		// A test player is built without a body, so it is given the one slot.
		thief->set_body_plan({ EquipmentSlot::BODY });
		thief->wear(
			ItemCreator::create("padded_armor", thief->position, ctx),
			EquipmentSlot::BODY);
		ASSERT_EQ(thief->thief_skill(ThiefSkill::PICK_POCKETS), 0);
		Creature& victim = add_victim_with_dagger(1);

		EXPECT_EQ(try_with_roll(victim, 1), PickPocketAttempt::NOT_A_THIEF);
	}

	TEST_F(PickPocketTest, AFullPackRefusesBeforeAnyRoll)
	{
		buy_pick_pockets(30);
		Creature& victim = add_victim_with_dagger(1);
		thief->inventoryData.capacity = 0;
		ASSERT_TRUE(
			InventoryOperations::is_inventory_full(thief->inventoryData));

		EXPECT_EQ(try_with_roll(victim, 1), PickPocketAttempt::PACK_FULL);
		EXPECT_EQ(victim.inventoryData.items.size(), 1u);
	}

	// An item the thief cannot carry stays where it was.
	TEST_F(PickPocketTest, AnItemTooHeavyToCarryStaysWithTheVictim)
	{
		buy_pick_pockets(30);
		thief->set_strength(3);
		Creature& victim = add_creature_at(Vector2D{ 6, 5 }, 1);
		give(victim, "plate_mail");
		ASSERT_FALSE(
			InventoryOperations::is_within_weight_limit(
				*victim.inventoryData.items.front(),
				*thief,
				*ctx.dataManager))
			<< "plate mail fits a Strength 3 thief, so nothing is measured";

		EXPECT_EQ(try_with_roll(victim, 10), PickPocketAttempt::TOOK_NOTHING);
		EXPECT_EQ(victim.inventoryData.items.size(), 1u);
	}

	// Beside means the eight neighbours; carrying means something to take.
	TEST_F(PickPocketTest, VictimsAreTheNeighboursCarryingSomething)
	{
		Creature& beside = add_victim_with_dagger(1);
		Creature& diagonal = add_creature_at(Vector2D{ 4, 4 }, 1);
		give(diagonal, "dagger");
		add_creature_at(Vector2D{ 4, 5 }, 1);
		Creature& distant = add_creature_at(Vector2D{ 7, 5 }, 1);
		give(distant, "dagger");

		const std::vector<Creature*> victims =
			pick_pocket_victims(*thief, creatures);

		EXPECT_EQ(victims, (std::vector<Creature*>{ &beside, &diagonal }));
	}

	// A body still lying beside the thief has nobody left to rob.
	TEST_F(PickPocketTest, TheDeadAreNoVictims)
	{
		Creature& corpse = add_victim_with_dagger(1);
		corpse.healthPool->set_hp(0);
		ASSERT_TRUE(corpse.is_dead());

		EXPECT_TRUE(pick_pocket_victims(*thief, creatures).empty());
	}

	// A try that came away empty still took the round.
	TEST_F(PickPocketTest, TheCommandSpendsARoundOnAFailedTry)
	{
		buy_pick_pockets(30);
		add_victim_with_dagger(1);
		const int before = ctx.gameState->get_time();

		mock.dice.set_next_roll(51);
		pick_pocket(ctx);

		EXPECT_FALSE(thief_holds("dagger"));
		EXPECT_EQ(
			ctx.gameState->get_time() - before,
			scaled_cost(TIME_UNITS_PER_ROUND, thief->get_action_delay()));
	}

	// The key's command charges a round for a try.
	TEST_F(PickPocketTest, TheCommandSpendsARoundOnATry)
	{
		buy_pick_pockets(30);
		Creature& victim = add_victim_with_dagger(1);
		const int before = ctx.gameState->get_time();

		mock.dice.set_next_roll(10);
		pick_pocket(ctx);

		EXPECT_TRUE(thief_holds("dagger"));
		EXPECT_TRUE(victim.inventoryData.items.empty());
		EXPECT_EQ(
			ctx.gameState->get_time() - before,
			scaled_cost(TIME_UNITS_PER_ROUND, thief->get_action_delay()));
	}

	TEST_F(PickPocketTest, TheCommandSpendsNothingWhenNobodyCarriesAnything)
	{
		buy_pick_pockets(30);
		add_creature_at(Vector2D{ 6, 5 }, 1);
		const int before = ctx.gameState->get_time();

		pick_pocket(ctx);

		EXPECT_EQ(ctx.gameState->get_time(), before);
	}

	TEST_F(PickPocketTest, TheCommandSpendsNothingForAClassWithNoScore)
	{
		thief->set_creature_class(CreatureClass::FIGHTER);
		Creature& victim = add_victim_with_dagger(1);
		const int before = ctx.gameState->get_time();

		mock.dice.set_next_roll(1);
		pick_pocket(ctx);

		EXPECT_EQ(ctx.gameState->get_time(), before);
		EXPECT_EQ(victim.inventoryData.items.size(), 1u);
	}

	// Two victims in reach: the choice is the player's, so a menu opens and
	// nothing is spent until it is answered.
	TEST_F(PickPocketTest, TheCommandAsksWhichWhenSeveralAreInReach)
	{
		buy_pick_pockets(30);
		add_victim_with_dagger(1);
		Creature& other = add_creature_at(Vector2D{ 4, 5 }, 1);
		give(other, "dagger");
		// ListMenu measures its text and BaseMenu asserts an input system. No
		// window is open, so the text measures zero, which is enough to build
		// the menu and count it.
		ctx.renderer = &renderer;
		ctx.inputSystem = &input;
		const size_t menusBefore = ctx.menus->size();
		const int before = ctx.gameState->get_time();

		pick_pocket(ctx);

		EXPECT_EQ(ctx.menus->size(), menusBefore + 1);
		EXPECT_EQ(ctx.gameState->get_time(), before);
	}

} // namespace CppRL
