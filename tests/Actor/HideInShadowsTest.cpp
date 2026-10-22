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
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

#include "src/ArmorClass.h"
#include "src/BuffSystem.h"
#include "src/Colors.h"
#include "src/Creature.h"
#include "src/ExperienceReward.h"
#include "src/GameContext.h"
#include "src/HealthPool.h"
#include "src/LogMessage.h"
#include "src/Map.h"
#include "src/Player.h"
#include "src/PlayerAttacker.h"
#include "src/ThiefSkills.h"
#include "src/TurnSchedule.h"
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
			map.set_tile(Vector2D{ col, 5 }, TileType::FLOOR);
		}

		thief->position = Vector2D{ 2, 5 };
		thief->healthPool = std::make_unique<HealthPool>(20);
		thief->armorClass = std::make_unique<ArmorClass>(10);
		thief->set_creature_class(CreatureClass::ROGUE);
		thief->lifespan->race = PlayerRaceState::HUMAN;
		// Dexterity 13 is the middle of Table 28's flat row, so the percentage is the
		// base score and the points, with nothing else folded in.
		thief->set_dexterity(13);
		thief->attacker = std::make_unique<PlayerAttacker>(*thief);
		thief->experienceReward = std::make_unique<ExperienceReward>(0);
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

	// How many stored messages hold the given text, their coloured runs joined.
	size_t times_said(std::string_view wanted) const
	{
		size_t count = 0;
		for (size_t index = 0; index < ctx.messageSystem->get_stored_message_count(); ++index)
		{
			std::string whole;
			for (const LogMessage& run : ctx.messageSystem->get_attack_message_at(index))
			{
				whole += run.text;
			}
			if (whole.find(wanted) != std::string::npos)
			{
				++count;
			}
		}
		return count;
	}

	// One hide with the secret roll forced, which must reach the roll.
	void hide_with_roll(int roll)
	{
		mock.dice.set_next_roll(roll);
		ASSERT_EQ(thief->attempt_hide(ctx), Player::HideAttempt::ATTEMPTED);
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
	thief->leave_shadows(ctx);

	mock.dice.set_next_roll(100);
	const Player::HideAttempt missed = thief->attempt_hide(ctx);

	EXPECT_EQ(hidden, missed);
}

// The skill belongs to the thief, and the book gives nobody else a way into the
// shadows.
TEST_F(HideInShadowsTest, AClassWithNoThiefSkillCannotTry)
{
	thief->set_creature_class(CreatureClass::FIGHTER);

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
	ASSERT_TRUE(guard.is_aware(ctx.gameState->get_time())) << "the guard is standing two tiles away in an open corridor";

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
	ASSERT_TRUE(guard.is_aware(ctx.gameState->get_time()));

	// The thief backs away down the corridor; the guard keeps its memory a while.
	thief->position = Vector2D{ 18, 5 };
	map.compute_fov(ctx);
	ASSERT_FALSE(map.is_in_fov(guard.position)) << "the thief cannot see the guard any more";
	ASSERT_TRUE(guard.is_aware(ctx.gameState->get_time())) << "and the guard has not forgotten him";

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
	ASSERT_FALSE(unnoticing.is_aware(ctx.gameState->get_time())) << "and it has not noticed him";

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
	ASSERT_TRUE(guard.is_aware(ctx.gameState->get_time()));
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

// "He remains hidden only as long as he remains virtually motionless" (PHB page 87):
// a step off the square he hid on ends the hide.
TEST_F(HideInShadowsTest, AStepOffTheHidingSquareEndsTheHide)
{
	buy_hide_in_shadows(40);
	hide_with_roll(1);
	ASSERT_TRUE(thief->is_invisible());

	thief->position = Vector2D{ 3, 5 };
	thief->leave_shadows_if_moved(ctx);

	EXPECT_FALSE(thief->is_invisible()) << "a thief who moved is no longer hidden";
	EXPECT_FALSE(thief->is_in_shadows()) << "and no longer believes he is";
}

// Standing still is the whole of the rule, so a thief who has not moved keeps his hide.
TEST_F(HideInShadowsTest, AThiefWhoStaysPutStaysHidden)
{
	buy_hide_in_shadows(40);
	hide_with_roll(1);

	thief->leave_shadows_if_moved(ctx);

	EXPECT_TRUE(thief->is_invisible());
	EXPECT_TRUE(thief->is_in_shadows());
}

// The book gives a hide no duration. The bound is the game's old invented one, 10 rounds
// and 2 a level, plus five rounds of margin: this says the hide outlasts the old cap, and
// states no new rule.
TEST_F(HideInShadowsTest, AHideOutlastsTheOldInventedDuration)
{
	buy_hide_in_shadows(40);
	hide_with_roll(1);

	const int oldCapRounds = 10 + thief->get_creature_level() * 2;
	ctx.buffSystem->update_creature_buffs(*thief, expiry_time(ctx.gameState->get_time(), oldCapRounds + 5));

	EXPECT_TRUE(thief->is_invisible()) << "a hide has no clock; only moving or attacking ends it";
}

// The spell is not the hide: PHB page 291 lets an invisible creature "open doors, talk,
// eat, climb stairs", so a thief under the spell who also hid keeps the spell on moving.
TEST_F(HideInShadowsTest, SpellInvisibilitySurvivesTheStepThatEndsAHide)
{
	buy_hide_in_shadows(40);
	// The hide comes first, because an invisible thief's try is refused as already hidden.
	hide_with_roll(100);
	ctx.buffSystem->add_buff(*thief, BuffType::INVISIBILITY, 0, 20, BuffApplication::ADD, ctx.gameState->get_time());

	thief->position = Vector2D{ 3, 5 };
	thief->leave_shadows_if_moved(ctx);

	EXPECT_FALSE(thief->is_in_shadows()) << "the hide ended";
	EXPECT_TRUE(thief->is_invisible()) << "and the spell did not end with it";
}

// A second try after a failed roll is answered as after a successful one, or the second
// press would tell the player the first one failed.
TEST_F(HideInShadowsTest, AFailedHideIsAlreadyHiddenToo)
{
	buy_hide_in_shadows(40);
	hide_with_roll(100);
	ASSERT_FALSE(thief->is_invisible()) << "the roll failed";

	// A certain success is queued and must not be reached.
	mock.dice.set_next_roll(1);
	EXPECT_EQ(thief->attempt_hide(ctx), Player::HideAttempt::ALREADY_HIDDEN);
}

// Stepping out says one line, and the same line whichever way the secret roll went.
TEST_F(HideInShadowsTest, AStepSaysTheSameWhicheverWayTheRollWent)
{
	buy_hide_in_shadows(40);
	const auto lines_on_stepping_out_after = [this](int roll)
	{
		thief->position = Vector2D{ 2, 5 };
		hide_with_roll(roll);
		const size_t before = times_said("You leave the shadows.");
		thief->position = Vector2D{ 3, 5 };
		thief->leave_shadows_if_moved(ctx);
		return times_said("You leave the shadows.") - before;
	};

	EXPECT_EQ(lines_on_stepping_out_after(1), 1u) << "a hide that worked";
	EXPECT_EQ(lines_on_stepping_out_after(100), 1u) << "a hide that did not";
}

// Striking from the shadows ends the hide and says so, again the same either way. The
// spell's "Your invisibility fades" line would be printed only for a roll that worked.
TEST_F(HideInShadowsTest, AnAttackSaysTheSameWhicheverWayTheRollWent)
{
	buy_hide_in_shadows(40);
	Creature& target = add_monster_at(Vector2D{ 3, 5 });
	target.healthPool = std::make_unique<HealthPool>(1000);
	target.armorClass = std::make_unique<ArmorClass>(10);
	// A test creature's Strength starts at zero, which no swing accepts.
	thief->set_strength(10);

	const auto lines_on_attacking_after = [this, &target](int roll)
	{
		hide_with_roll(roll);
		const size_t leftBefore = times_said("You leave the shadows.");
		const size_t fadedBefore = times_said("Your invisibility fades");
		thief->attacker->attack(target, AttackKind::MELEE, ctx);
		EXPECT_FALSE(thief->is_in_shadows()) << "roll " << roll;
		EXPECT_FALSE(thief->is_invisible()) << "roll " << roll;
		EXPECT_EQ(times_said("Your invisibility fades"), fadedBefore) << "roll " << roll;
		return times_said("You leave the shadows.") - leftBefore;
	};

	EXPECT_EQ(lines_on_attacking_after(1), 1u) << "a hide that worked";
	EXPECT_EQ(lines_on_attacking_after(100), 1u) << "a hide that did not";
}

// A saved hide is still a hide when the game is loaded, on the same square.
TEST_F(HideInShadowsTest, AHideSurvivesASaveAndLoad)
{
	buy_hide_in_shadows(40);
	hide_with_roll(1);

	nlohmann::json record;
	thief->save(record);
	Player loaded{ Vector2D{ 0, 0 } };
	loaded.load(record);
	// A loaded buff imposes its state at the first update, as the game's first turn does.
	ctx.buffSystem->update_creature_buffs(loaded, ctx.gameState->get_time());

	EXPECT_TRUE(loaded.is_invisible());
	EXPECT_TRUE(loaded.is_in_shadows());
	loaded.leave_shadows_if_moved(ctx);
	EXPECT_TRUE(loaded.is_in_shadows()) << "the spot came back with the player, so he has not moved";
}

// end of file: HideInShadowsTest.cpp
