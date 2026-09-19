// file: ConstitutionRegenerationTest.cpp
// Table 3's regeneration column against the book (Player's Handbook, PDF page 33):
// "The character heals 1 point of damage after the passage of the listed number of
// turns. However, fire and acid damage ... cannot be regenerated in this manner.
// These injuries must heal normally or be dealt with by magical means."
//
// The listed turns: 6 at Constitution 20, then 5, 4, 3, 2, and 1 at 25; nothing
// below 20. "Ten combat rounds equal one turn" (PDF page 24), and a game turn is a
// round, so Constitution 20 heals a point every 60 rounds and 25 every 10.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=ConstitutionRegenerationTest.*

#include <gtest/gtest.h>

#include <array>
#include <memory>

#include "src/Creature.h"
#include "src/CreatureClass.h"
#include "src/DamageInfo.h"
#include "src/DamageResolver.h"
#include "src/GameContext.h"
#include "src/HealthPool.h"
#include "tests/mocks/MockGameContext.h"

namespace
{

constexpr int ROUNDS_WATCHED = 120;

} // namespace

class ConstitutionRegenerationTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();
	}

	// A creature of this class and Constitution with maxHp hit points, down to hp.
	std::unique_ptr<Creature> make_creature(CreatureClass creatureClass, int constitution, int maxHp, int hp)
	{
		auto creature = std::make_unique<Creature>(Vector2D{ 0, 0 }, ActorData{ TileRef{}, "hero", 0 });
		creature->healthPool = std::make_unique<HealthPool>(maxHp);
		creature->set_creature_class(creatureClass);
		creature->set_constitution(constitution);
		creature->set_hp(hp);
		return creature;
	}

	// Runs rounds 1 through `rounds` of regeneration and returns the hit points gained.
	int regenerate_through(Creature& creature, int rounds)
	{
		const int before = creature.get_hp();
		for (int round = 1; round <= rounds; ++round)
		{
			creature.regenerate_from_constitution(round, mock.data_manager);
		}
		return creature.get_hp() - before;
	}

	MockGameContext mock{};
	GameContext ctx{};
};

// A point after each listed number of turns, and not a round sooner.
TEST_F(ConstitutionRegenerationTest, EachScoreHealsAPointEveryListedTurn)
{
	struct RegenerationRow
	{
		int constitution{ 0 };
		int turnsBetweenPoints{ 0 };
	};
	constexpr std::array<RegenerationRow, 6> TABLE_THREE{ {
		{ 20, 6 },
		{ 21, 5 },
		{ 22, 4 },
		{ 23, 3 },
		{ 24, 2 },
		{ 25, 1 },
	} };

	for (const RegenerationRow& row : TABLE_THREE)
	{
		const int roundsBetweenPoints = row.turnsBetweenPoints * 10;

		auto early = make_creature(CreatureClass::FIGHTER, row.constitution, 200, 100);
		EXPECT_EQ(regenerate_through(*early, roundsBetweenPoints - 1), 0) << "Constitution " << row.constitution;

		auto watched = make_creature(CreatureClass::FIGHTER, row.constitution, 200, 100);
		EXPECT_EQ(regenerate_through(*watched, ROUNDS_WATCHED), ROUNDS_WATCHED / roundsBetweenPoints) << "Constitution " << row.constitution;
	}
}

// Below 20 the column is "Nil".
TEST_F(ConstitutionRegenerationTest, NineteenRegeneratesNothing)
{
	auto character = make_creature(CreatureClass::FIGHTER, 19, 200, 100);

	EXPECT_EQ(regenerate_through(*character, ROUNDS_WATCHED), 0);
}

// A monster takes no Constitution hit points, so no regeneration from it either.
TEST_F(ConstitutionRegenerationTest, AMonsterDoesNotRegenerate)
{
	auto monster = make_creature(CreatureClass::MONSTER, 25, 200, 100);

	EXPECT_EQ(regenerate_through(*monster, ROUNDS_WATCHED), 0);
}

// Of 6 acid, 4 fire and 5 slashing, only the slashing comes back.
TEST_F(ConstitutionRegenerationTest, FireAndAcidAreNotRegenerated)
{
	auto character = make_creature(CreatureClass::FIGHTER, 25, 20, 20);
	character->take_damage(6, ctx, DamageType::ACID);
	character->take_damage(DamageResolver::reduce_dice({ 4 }, DamageType::FIRE, 0), ctx);
	character->take_damage(5, ctx, DamageType::PHYSICAL);
	ASSERT_EQ(character->get_hp(), 5);

	EXPECT_EQ(regenerate_through(*character, 200), 5);
	EXPECT_EQ(character->get_unregenerable_damage(), 10);
}

// Other healing heals the fire and acid first, which leaves regeneration the rest.
TEST_F(ConstitutionRegenerationTest, OtherHealingTakesTheFireAndAcidFirst)
{
	auto character = make_creature(CreatureClass::FIGHTER, 25, 20, 20);
	character->take_damage(6, ctx, DamageType::ACID);
	character->take_damage(4, ctx, DamageType::PHYSICAL);

	EXPECT_EQ(character->heal(6), 6);
	EXPECT_EQ(character->get_unregenerable_damage(), 0);
	EXPECT_EQ(regenerate_through(*character, 40), 4);
	EXPECT_EQ(character->get_hp(), 20);
}

// Healing less than the fire and acid leaves the remainder unregenerable.
TEST_F(ConstitutionRegenerationTest, PartialHealingLeavesTheRestOfTheAcid)
{
	auto character = make_creature(CreatureClass::FIGHTER, 25, 20, 20);
	character->take_damage(6, ctx, DamageType::ACID);

	EXPECT_EQ(character->heal(2), 2);
	EXPECT_EQ(character->get_unregenerable_damage(), 4);
	EXPECT_EQ(regenerate_through(*character, 100), 0);
}

// The fire and acid are part of the damage taken, never more of it: 15 acid and then
// 10 more on a 20-point creature leave all 20 lost points as acid, not 25.
TEST_F(ConstitutionRegenerationTest, FireAndAcidNeverExceedTheDamageTaken)
{
	auto character = make_creature(CreatureClass::FIGHTER, 25, 20, 20);
	character->take_damage(15, ctx, DamageType::ACID);
	character->take_damage(10, ctx, DamageType::ACID);

	EXPECT_EQ(character->get_hp(), 0);
	EXPECT_EQ(character->get_unregenerable_damage(), 20);
}

// Regeneration heals wounds; it does not raise the dead.
TEST_F(ConstitutionRegenerationTest, TheDeadDoNotRegenerate)
{
	auto character = make_creature(CreatureClass::FIGHTER, 25, 20, 0);

	EXPECT_EQ(regenerate_through(*character, 100), 0);
}

// A save keeps what regeneration cannot reach.
TEST_F(ConstitutionRegenerationTest, TheFireAndAcidSurviveASave)
{
	auto character = make_creature(CreatureClass::FIGHTER, 25, 20, 20);
	character->take_damage(6, ctx, DamageType::ACID);
	json saved;
	character->save(saved);

	auto loaded = make_creature(CreatureClass::FIGHTER, 25, 1, 1);
	loaded->load(saved);

	EXPECT_EQ(loaded->get_unregenerable_damage(), 6);
}
