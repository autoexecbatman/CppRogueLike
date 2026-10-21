// file: RangedDrawTest.cpp
// A ranged monster too weak to draw its bow fights as a melee monster instead of trying
// to shoot every round.
//
// What it is for. The composite bow needs Strength 18 to draw (the owner's Baldur's Gate
// ruling, read through can_draw). The shot itself is refused at the attack, so a ranged
// AI that chose to shoot anyway spent each turn printing "is not strong enough to draw"
// and never closed in. The AI asks can_draw before choosing, each turn, because Strength
// is read as it stands now.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=RangedDrawTest.*

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "src/AiMonsterRanged.h"
#include "src/ArmorClass.h"
#include "src/Colors.h"
#include "src/Creature.h"
#include "src/DamageInfo.h"
#include "src/EquipmentSlot.h"
#include "src/ExperienceReward.h"
#include "src/Game.h"
#include "src/HealthPool.h"
#include "src/ItemCreator.h"
#include "src/LogMessage.h"
#include "src/Map.h"
#include "src/MessageSystem.h"
#include "src/MonsterAttacker.h"
#include "src/Paths.h"
#include "src/Pickable.h"
#include "src/Player.h"
#include "src/PlayerAttacker.h"
#include "src/RandomDice.h"
#include "tests/mocks/TestDice.h"

namespace
{

constexpr int STARTING_HP = 100;
constexpr int TOO_WEAK_TO_DRAW = 12;
constexpr int STRONG_ENOUGH_TO_DRAW = 18;

// Within the player's field of view and inside the archer's range of 2 to 5, so the
// ranged branch is the one deciding; adjacent is the melee branch's distance.
constexpr int PLAYER_COLUMN = 4;
constexpr int IN_RANGE_COLUMN = 7;
constexpr int ADJACENT_COLUMN = 5;
constexpr int CORRIDOR_ROW = 5;

} // namespace

class RangedDrawTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		game.dataManager.load_all_data(game.messageSystem);
		game.tileConfig.load(Paths::TILE_CONFIG);
		game.itemRegistry.load(Paths::ITEMS);

		player->experienceReward = std::make_unique<ExperienceReward>(0);
		player->healthPool = std::make_unique<HealthPool>(STARTING_HP);
		player->armorClass = std::make_unique<ArmorClass>(10);
		player->attacker = std::make_unique<PlayerAttacker>(*player);
		player->set_dr(0);
		player->set_thaco(20);
		player->set_strength(10);
		player->set_dexterity(10);

		archer.experienceReward = std::make_unique<ExperienceReward>(0);
		archer.healthPool = std::make_unique<HealthPool>(STARTING_HP);
		archer.armorClass = std::make_unique<ArmorClass>(10);
		archer.attacker = std::make_unique<MonsterAttacker>(archer, DamageInfo{ "1d4", DamageType::PHYSICAL });
		archer.set_dr(0);
		archer.set_thaco(20);
		archer.set_dexterity(10);
		archer.set_natural_attack("fist");
		archer.set_body_plan({ EquipmentSlot::RIGHT_HAND, EquipmentSlot::MISSILE_WEAPON });
		archer.ai = std::make_unique<AiMonsterRanged>();

		ctx = game.context();
		ctx.playerOwner = &player;
		ctx.map = &map;
		ctx.creatures = &creatures;

		// Worn the way MonsterCreator puts authored gear on, which asks no Strength.
		archer.wear(ItemCreator::create("composite_bow", archer.position, ctx), EquipmentSlot::MISSILE_WEAPON);

		// One open corridor, so any change in the archer's column is it closing or backing.
		map.init_tiles();
		for (int column = 1; column < 19; ++column)
		{
			map.set_tile(Vector2D{ column, CORRIDOR_ROW }, TileType::FLOOR);
		}
		map.compute_fov(ctx);

		// The top of every die: no idle round starts, the morale check holds, every blow lands.
		game.dice.set_test_mode(true);
		for (int queued = 0; queued < 60; ++queued)
		{
			game.dice.set_next_roll(RandomDice::HIGHEST);
		}
	}

	void TearDown() override
	{
		game.dice.set_test_mode(false);
		game.dice.clear_fixed_rolls();
	}

	bool any_message_says(const std::string& phrase) const
	{
		for (size_t index = 0; index < game.messageSystem.get_stored_message_count(); ++index)
		{
			for (const LogMessage& part : game.messageSystem.get_attack_message_at(index))
			{
				if (part.text.find(phrase) != std::string::npos)
				{
					return true;
				}
			}
		}
		return false;
	}

	Game game{ TEST_DICE_SEED };
	GameContext ctx;
	Map map{ 20, 20 };
	std::vector<std::unique_ptr<Creature>> creatures{};
	std::unique_ptr<Player> player{ std::make_unique<Player>(Vector2D{ PLAYER_COLUMN, CORRIDOR_ROW }) };
	Creature archer{ Vector2D{ IN_RANGE_COLUMN, CORRIDOR_ROW }, ActorData{ TileRef{}, "archer", ColorPairId::WHITE_BLACK } };
};

TEST_F(RangedDrawTest, AnArcherTooWeakForItsBowClosesInInsteadOfShooting)
{
	archer.set_strength(TOO_WEAK_TO_DRAW);
	ASSERT_FALSE(can_draw(archer, *archer.get_equipped_item(EquipmentSlot::MISSILE_WEAPON))) << "the bow can be drawn, so nothing here is too weak";

	archer.ai->update(archer, ctx);

	EXPECT_FALSE(any_message_says("not strong enough to draw")) << "the archer chose a shot it cannot draw";
	EXPECT_EQ(archer.position.x, IN_RANGE_COLUMN - 1) << "the archer stood its ground instead of closing in";
}

TEST_F(RangedDrawTest, AnArcherTooWeakForItsBowFightsInMeleeWhenAdjacent)
{
	archer.set_strength(TOO_WEAK_TO_DRAW);
	archer.position = Vector2D{ ADJACENT_COLUMN, CORRIDOR_ROW };

	archer.ai->update(archer, ctx);

	EXPECT_EQ(archer.position.x, ADJACENT_COLUMN) << "the archer backed away to make room for a shot it cannot draw";
	EXPECT_LT(player->get_hp(), STARTING_HP) << "the archer did not strike the creature beside it";
}

TEST_F(RangedDrawTest, AnArcherStrongEnoughForItsBowShootsFromRange)
{
	archer.set_strength(STRONG_ENOUGH_TO_DRAW);

	archer.ai->update(archer, ctx);

	EXPECT_EQ(archer.position.x, IN_RANGE_COLUMN) << "an archer that can draw its bow walked in instead of shooting";
	EXPECT_LT(player->get_hp(), STARTING_HP) << "an archer that can draw its bow did not hit with it";
}

TEST_F(RangedDrawTest, AnArcherWithNoBowClosesIn)
{
	archer.set_strength(STRONG_ENOUGH_TO_DRAW);
	ASSERT_TRUE(archer.unequip_item(EquipmentSlot::MISSILE_WEAPON, ctx));
	ASSERT_EQ(archer.get_equipped_item(EquipmentSlot::MISSILE_WEAPON), nullptr) << "the bow is still worn, so nothing here lacks one";

	archer.ai->update(archer, ctx);

	EXPECT_EQ(archer.position.x, IN_RANGE_COLUMN - 1) << "an archer with no bow stood off to shoot one";
}

TEST_F(RangedDrawTest, StrengthLostBetweenTurnsIsReadOnTheNextTurn)
{
	archer.set_strength(STRONG_ENOUGH_TO_DRAW);
	archer.ai->update(archer, ctx);
	ASSERT_EQ(archer.position.x, IN_RANGE_COLUMN) << "the first turn did not shoot, so the change is never seen";

	archer.set_strength(TOO_WEAK_TO_DRAW);
	archer.ai->update(archer, ctx);

	EXPECT_EQ(archer.position.x, IN_RANGE_COLUMN - 1) << "the archer kept the answer from a Strength it no longer has";
}
