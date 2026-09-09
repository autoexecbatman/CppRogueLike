// file: TurnUndeadTest.cpp
// Verifies who may turn undead, which creatures an attempt reaches, and what
// each outcome does. Table 61 itself is checked in TurningTableTest; this covers
// the system built on it.

#include <gtest/gtest.h>

#include "src/Actor/Creature.h"
#include "src/ActorTypes/Player.h"
#include "src/Combat/ExperienceReward.h"
#include "src/Combat/TurnUndead.h"
#include "src/Utils/Vector2D.h"
#include "tests/mocks/MockGameContext.h"

class TurnUndeadTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();
		ctx.playerOwner = &player;
		ctx.creatures = &creatures;

		player->healthPool = std::make_unique<HealthPool>(20);
		player->armorClass = std::make_unique<ArmorClass>(10);

		// A destroyed undead reaches Creature::die, which pays the player its
		// kill reward, so the player needs one too.
		player->experienceReward = std::make_unique<ExperienceReward>(0);
		player->playerClassState = Player::PlayerClassState::CLERIC;
		player->set_creature_level(7);
	}

	// Places one undead near the player. Hit dice come from creature level,
	// which is the row a turning attempt reads.
	Creature& add_undead(int hitDice, Vector2D position)
	{
		auto undead = std::make_unique<Creature>(position, ActorData{ TileRef{}, "skeleton", 0 });
		undead->healthPool = std::make_unique<HealthPool>(8);

		// Every creature the game builds has one; die() reads it for the kill reward.
		undead->experienceReward = std::make_unique<ExperienceReward>(0);
		undead->set_undead(true);
		undead->set_creature_level(hitDice);
		creatures.push_back(std::move(undead));
		return *creatures.back();
	}

	Creature& add_living(Vector2D position)
	{
		auto living = std::make_unique<Creature>(position, ActorData{ TileRef{}, "goblin", 0 });
		living->healthPool = std::make_unique<HealthPool>(8);
		creatures.push_back(std::move(living));
		return *creatures.back();
	}

	void force_next_roll(int value)
	{
		mock.dice.set_next_roll(value);
	}

	MockGameContext mock{};
	GameContext ctx{};
	std::unique_ptr<Player> player{ std::make_unique<Player>(Vector2D{ 5, 5 }) };
	std::vector<std::unique_ptr<Creature>> creatures{};
};

// Only a cleric channels a deity.
TEST_F(TurnUndeadTest, NonClericCannotAttempt)
{
	player->playerClassState = Player::PlayerClassState::FIGHTER;
	add_undead(1, Vector2D{ 6, 5 });

	const TurnUndeadReport report = turn_undead(*player, ctx);

	EXPECT_FALSE(report.attempted);
	EXPECT_TRUE(report.turned.empty());
	EXPECT_TRUE(report.destroyed.empty());
}

// A 7th-level priest destroys a 1 HD skeleton outright: Table 61 gives "D".
TEST_F(TurnUndeadTest, StrongPriestDestroysWeakUndead)
{
	Creature& skeleton = add_undead(1, Vector2D{ 6, 5 });
	force_next_roll(10); // the d20; irrelevant against a D result
	force_next_roll(6); // 2d6 affected

	const TurnUndeadReport report = turn_undead(*player, ctx);

	EXPECT_TRUE(report.attempted);
	ASSERT_EQ(report.destroyed.size(), 1u);
	EXPECT_EQ(report.destroyed.front(), &skeleton);
	EXPECT_TRUE(skeleton.is_dead());
}

// A wight needs a 4 at level 7; the roll clears it, so the wight flees.
TEST_F(TurnUndeadTest, SuccessfulRollSetsTheUndeadFleeing)
{
	Creature& wight = add_undead(5, Vector2D{ 6, 5 });
	force_next_roll(12);
	force_next_roll(6);

	const TurnUndeadReport report = turn_undead(*player, ctx);

	ASSERT_EQ(report.turned.size(), 1u);
	EXPECT_EQ(report.turned.front(), &wight);
	EXPECT_TRUE(wight.has_state(ActorState::IS_FLEEING));
	EXPECT_FALSE(wight.is_dead()) << "turning drives off, it does not kill";
}

// A spectre needs a 16 at level 7; a 12 falls short and it is unmoved.
TEST_F(TurnUndeadTest, FailedRollLeavesTheUndeadUnmoved)
{
	Creature& spectre = add_undead(8, Vector2D{ 6, 5 });
	force_next_roll(12);
	force_next_roll(6);

	const TurnUndeadReport report = turn_undead(*player, ctx);

	ASSERT_EQ(report.resisted.size(), 1u);
	EXPECT_EQ(report.resisted.front(), &spectre);
	EXPECT_FALSE(spectre.has_state(ActorState::IS_FLEEING));
}

// Living creatures are not undead and are never touched.
TEST_F(TurnUndeadTest, LivingCreaturesAreIgnored)
{
	Creature& goblin = add_living(Vector2D{ 6, 5 });
	force_next_roll(20);
	force_next_roll(6);

	const TurnUndeadReport report = turn_undead(*player, ctx);

	EXPECT_TRUE(report.turned.empty());
	EXPECT_TRUE(report.destroyed.empty());
	EXPECT_TRUE(report.resisted.empty());
	EXPECT_FALSE(goblin.has_state(ActorState::IS_FLEEING));
}

// An undead beyond the priest's reach is not affected.
TEST_F(TurnUndeadTest, UndeadOutOfRangeIsUnaffected)
{
	Creature& distant = add_undead(1, Vector2D{ 5 + TURN_UNDEAD_RANGE + 1, 5 });
	force_next_roll(20);
	force_next_roll(6);

	const TurnUndeadReport report = turn_undead(*player, ctx);

	EXPECT_TRUE(report.destroyed.empty());
	EXPECT_FALSE(distant.is_dead());
}

// The book rolls once for the whole attempt and reads it per type, so a single
// roll can turn one undead and fail against another.
TEST_F(TurnUndeadTest, OneRollIsReadAgainstEveryType)
{
	Creature& wight = add_undead(5, Vector2D{ 6, 5 }); // needs 4
	Creature& spectre = add_undead(8, Vector2D{ 4, 5 }); // needs 16
	force_next_roll(12);
	force_next_roll(6);

	const TurnUndeadReport report = turn_undead(*player, ctx);

	EXPECT_EQ(report.roll, 12);
	ASSERT_EQ(report.turned.size(), 1u);
	EXPECT_EQ(report.turned.front(), &wight);
	ASSERT_EQ(report.resisted.size(), 1u);
	EXPECT_EQ(report.resisted.front(), &spectre);
}
