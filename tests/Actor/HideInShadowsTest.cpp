// file: HideInShadowsTest.cpp
// Melting into the shadows, by the Player's Handbook's Hide in Shadows (PDF page 87).
//
// The book's rule: "A thief can try to disappear into shadows or any other type of
// concealment... A thief can hide this way only when no one is looking at him... A
// thief can never become hidden while a guard is watching him, no matter what his dice
// roll is--his position is obvious to the guard... The DM rolls the dice and keeps the
// result secret, but the thief always thinks he is hidden."
//
// Three claims follow and none was true of the code this replaces: there is a roll at
// all, the roll is the character's own percentage, and the player cannot tell a failure
// from a success - which is why attempt_hide answers ATTEMPTED for both.
//
// Every roll is scripted. A roll left unscripted is a real one, so a case that expects
// no roll queues a certain success and checks it was not spent.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=HideInShadowsTest.*

#include <gtest/gtest.h>

#include <memory>

#include "src/Colors.h"
#include "src/ArmorClass.h"
#include "src/BuffSystem.h"
#include "src/Creature.h"
#include "src/GameContext.h"
#include "src/HealthPool.h"
#include "src/Map.h"
#include "src/Player.h"
#include "src/ThiefSkills.h"
#include "tests/mocks/MockGameContext.h"

class HideInShadowsTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();
		ctx.playerOwner = &thief;
		ctx.map = &map;
		ctx.creatures = &creatures;

		map.init_tiles();
		// An open corridor, so what a creature knows is a matter of awareness rather
		// than of walls.
		for (int col = 1; col < 19; ++col)
		{
			map.set_tile(Vector2D{ col, 5 }, TileType::FLOOR, 1);
		}

		thief->position = Vector2D{ 2, 5 };
		thief->healthPool = std::make_unique<HealthPool>(20);
		thief->armorClass = std::make_unique<ArmorClass>(10);
		thief->set_creature_class(CreatureClass::ROGUE);
		thief->playerClassState = Player::PlayerClassState::ROGUE;
		thief->playerRaceState = Player::PlayerRaceState::HUMAN;
		// Dexterity 13 is the middle of Table 28's flat row, so the percentage is the
		// base score and the points, with nothing else folded in.
		thief->set_dexterity(13);
	}

	// Hide in Shadows is base 5, so this is what the character rolls against.
	void buy_hide_in_shadows(int points)
	{
		thief->thiefSkillPoints.at(thief_skill_index(ThiefSkill::HIDE_IN_SHADOWS)) = points;
	}

	// A monster in the corridor. Awareness is granted by the creature's own look,
	// so one that has not been given a look has not noticed the thief.
	Creature& add_monster_at(Vector2D where)
	{
		auto monster = std::make_unique<Creature>(where, ActorData{ TileRef{}, "goblin", ColorPairId::WHITE_BLACK });
		monster->healthPool = std::make_unique<HealthPool>(8);
		Creature& placed = *monster;
		creatures.push_back(std::move(monster));
		return placed;
	}

	// Lets every creature look, from wherever the thief is standing.
	void everyone_looks()
	{
		map.compute_fov(ctx);
		for (const auto& creature : creatures)
		{
			creature->update_awareness(ctx);
		}
	}

	MockGameContext mock{};
	GameContext ctx{};
	Map map{ 20, 12 };
	std::vector<std::unique_ptr<Creature>> creatures{};
	std::unique_ptr<Player> thief{ std::make_unique<Player>(Vector2D{ 2, 5 }) };
};

// Base 5, Table 29's +5 for wearing nothing, and 40 bought is 50, and the roll is
// against that: a 50 hides and a 51 does not. Nothing else in the game can produce
// those two numbers.
TEST_F(HideInShadowsTest, TheRollIsAgainstTheCharactersOwnPercentage)
{
	buy_hide_in_shadows(40);
	ASSERT_EQ(thief->thief_skill(ThiefSkill::HIDE_IN_SHADOWS), 50);

	mock.dice.set_next_roll(50);
	EXPECT_EQ(thief->attempt_hide(ctx), Player::HideAttempt::ATTEMPTED);
	EXPECT_TRUE(thief->is_invisible()) << "a roll on the percentage hides";
}

// "The DM rolls the dice and keeps the result secret, but the thief always thinks he
// is hidden": the answer is the same, and only the world knows the difference.
TEST_F(HideInShadowsTest, AFailedRollLooksExactlyLikeASuccess)
{
	buy_hide_in_shadows(40);

	mock.dice.set_next_roll(51);
	EXPECT_EQ(thief->attempt_hide(ctx), Player::HideAttempt::ATTEMPTED)
		<< "the answer must not tell the player the roll failed";
	EXPECT_FALSE(thief->is_invisible()) << "and yet nothing is hidden";
}

// A hide that failed still cost the turn, which is the other half of keeping it
// secret: a refusal the player can see is a free action.
TEST_F(HideInShadowsTest, BothOutcomesAreTheOneAnswerThatSpendsTheTurn)
{
	buy_hide_in_shadows(40);

	mock.dice.set_next_roll(1);
	const Player::HideAttempt hidden = thief->attempt_hide(ctx);
	ctx.buffSystem->remove_buff(*thief, BuffType::INVISIBILITY);

	mock.dice.set_next_roll(100);
	const Player::HideAttempt missed = thief->attempt_hide(ctx);

	EXPECT_EQ(hidden, missed);
}

// The skill belongs to the thief, and the book gives nobody else a way into the
// shadows.
TEST_F(HideInShadowsTest, AClassWithNoThiefSkillCannotTry)
{
	thief->set_creature_class(CreatureClass::FIGHTER);
	thief->playerClassState = Player::PlayerClassState::FIGHTER;

	// A certain success is queued and must not be reached.
	mock.dice.set_next_roll(1);
	EXPECT_EQ(thief->attempt_hide(ctx), Player::HideAttempt::NOT_A_THIEF);
	EXPECT_FALSE(thief->is_invisible());
}

// A score the adjustments leave at nothing is a skill not yet bought up to a usable
// percentage, which the book treats as not having it.
TEST_F(HideInShadowsTest, ARogueWhoHasNotBoughtTheSkillUpCannotTryEither)
{
	// Base 5, Dexterity 9 is -10 on this column: the score is nothing.
	thief->set_dexterity(9);
	buy_hide_in_shadows(0);
	ASSERT_EQ(thief->thief_skill(ThiefSkill::HIDE_IN_SHADOWS), 0);

	mock.dice.set_next_roll(1);
	EXPECT_EQ(thief->attempt_hide(ctx), Player::HideAttempt::NOT_A_THIEF);
	EXPECT_FALSE(thief->is_invisible());
}

// "A thief can never become hidden while a guard is watching him, no matter what his
// dice roll is." A creature that has the thief in mind is watching.
TEST_F(HideInShadowsTest, AWatchingCreatureRefusesTheAttemptWithoutARoll)
{
	buy_hide_in_shadows(40);
	Creature& guard = add_monster_at(Vector2D{ 4, 5 });
	everyone_looks();
	ASSERT_TRUE(guard.is_aware()) << "the guard is standing two tiles away in an open corridor";

	mock.dice.set_next_roll(1);
	EXPECT_EQ(thief->attempt_hide(ctx), Player::HideAttempt::WATCHED);
	EXPECT_FALSE(thief->is_invisible());
}

// A guard that has lost sight is still hunting, and the book's bar is whether anyone
// is looking for him rather than whether he can see them.
TEST_F(HideInShadowsTest, AHunterThatHasLostSightIsStillWatching)
{
	buy_hide_in_shadows(40);
	Creature& guard = add_monster_at(Vector2D{ 4, 5 });
	everyone_looks();
	ASSERT_TRUE(guard.is_aware());

	// The thief backs away down the corridor; the guard keeps its memory a while.
	thief->position = Vector2D{ 18, 5 };
	map.compute_fov(ctx);
	ASSERT_FALSE(map.is_in_fov(guard.position)) << "the thief cannot see the guard any more";
	ASSERT_TRUE(guard.is_aware()) << "and the guard has not forgotten him";

	mock.dice.set_next_roll(1);
	EXPECT_EQ(thief->attempt_hide(ctx), Player::HideAttempt::WATCHED);
}

// The question is whether anyone is looking, not whether the thief can see anyone: a
// monster that has never noticed him is no reason not to try.
TEST_F(HideInShadowsTest, ACreatureThatHasNotNoticedTheThiefIsNoBarrier)
{
	buy_hide_in_shadows(40);
	const Creature& unnoticing = add_monster_at(Vector2D{ 4, 5 });
	map.compute_fov(ctx);
	ASSERT_TRUE(map.is_in_fov(unnoticing.position)) << "the thief can see it plainly";
	ASSERT_FALSE(unnoticing.is_aware()) << "and it has not noticed him";

	mock.dice.set_next_roll(1);
	EXPECT_EQ(thief->attempt_hide(ctx), Player::HideAttempt::ATTEMPTED);
	EXPECT_TRUE(thief->is_invisible());
}

// A dead watcher is watching nothing.
TEST_F(HideInShadowsTest, ADeadCreatureDoesNotWatch)
{
	buy_hide_in_shadows(40);
	Creature& guard = add_monster_at(Vector2D{ 4, 5 });
	everyone_looks();
	ASSERT_TRUE(guard.is_aware());
	guard.set_hp(0);
	ASSERT_TRUE(guard.is_dead());

	mock.dice.set_next_roll(1);
	EXPECT_EQ(thief->attempt_hide(ctx), Player::HideAttempt::ATTEMPTED);
}

TEST_F(HideInShadowsTest, AThiefAlreadyInTheShadowsDoesNotTryAgain)
{
	buy_hide_in_shadows(40);
	mock.dice.set_next_roll(1);
	ASSERT_EQ(thief->attempt_hide(ctx), Player::HideAttempt::ATTEMPTED);
	ASSERT_TRUE(thief->is_invisible());

	// A certain success is queued and must not be reached.
	mock.dice.set_next_roll(1);
	EXPECT_EQ(thief->attempt_hide(ctx), Player::HideAttempt::ALREADY_HIDDEN);
}

// end of file: HideInShadowsTest.cpp
