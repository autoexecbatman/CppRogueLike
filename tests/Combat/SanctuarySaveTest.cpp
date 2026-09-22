// Checks that Sanctuary wards whoever bears it against anyone attacking them: each
// attacker saves once per casting, the result holds for as long as that casting does,
// and a turned-away attack lands nothing.
//
// What it is for. The Player's Handbook, PDF page 436 of the 2e archive: "any opponent
// attempting to strike or otherwise directly attack the protected creature must roll a
// saving throw vs. spell. If the saving throw is successful, the opponent can attack
// normally and is unaffected by that casting of the spell. If the saving throw is
// failed, the opponent loses track of and totally ignores the warded creature for the
// duration of the spell." The rule is about the protected creature and any opponent,
// so the tests ward the player and ward a monster, and attack with both.
//
// Each test queues rolls so that a save skipped, or rolled and then ignored, would land
// damage or flip the answer: the save's own roll also hits armour class 10, and a sure
// hit waits behind it.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=SanctuarySaveTest.*

#include <gtest/gtest.h>

#include <initializer_list>
#include <memory>

#include <nlohmann/json.hpp>

#include "src/ArmorClass.h"
#include "src/AttackKind.h"
#include "src/BuffSystem.h"
#include "src/BuffType.h"
#include "src/Creature.h"
#include "src/DamageInfo.h"
#include "src/ExperienceReward.h"
#include "src/Game.h"
#include "src/HealthPool.h"
#include "src/MonsterAttacker.h"
#include "src/Player.h"
#include "src/PlayerAttacker.h"
#include "src/SavingThrow.h"

class SanctuarySaveTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		game.dataManager.load_all_data(game.messageSystem);

		player->experienceReward = std::make_unique<ExperienceReward>(0);
		player->healthPool = std::make_unique<HealthPool>(STARTING_HP);
		player->armorClass = std::make_unique<ArmorClass>(10);
		player->attacker = std::make_unique<PlayerAttacker>(*player);
		player->set_dr(0);
		player->set_thaco(20);
		player->set_strength(10);
		player->set_dexterity(10);
		arm(goblin);
		arm(orc);

		ctx = game.context();
		ctx.playerOwner = &player;
		game.dice.set_test_mode(true);
	}

	void TearDown() override
	{
		game.dice.set_test_mode(false);
		game.dice.clear_fixed_rolls();
	}

	// A 1d4 bite from THAC0 20 at armour class 10; strength 8 adds nothing to damage.
	static void arm(Creature& monster)
	{
		monster.experienceReward = std::make_unique<ExperienceReward>(0);
		monster.healthPool = std::make_unique<HealthPool>(STARTING_HP);
		monster.armorClass = std::make_unique<ArmorClass>(10);
		monster.attacker = std::make_unique<MonsterAttacker>(monster, DamageInfo{ "1d4", DamageType::PHYSICAL });
		monster.set_dr(0);
		monster.set_thaco(20);
		monster.set_strength(8);
		monster.set_dexterity(10);
		monster.set_natural_attack("bite");
	}

	void cast_sanctuary(Creature& warded)
	{
		ctx.buffSystem->add_buff(warded, BuffType::SANCTUARY, 0, 10, false);
	}

	bool turned_away_from_player(const Creature& attacker)
	{
		return ctx.buffSystem->is_turned_away_by_sanctuary(attacker, *player, ctx);
	}

	// Queues rolls in the order the game asks for them.
	void script(std::initializer_list<int> rolls)
	{
		for (const int roll : rolls)
		{
			game.dice.set_next_roll(roll);
		}
	}

	static int hp_lost(const Creature& target)
	{
		return STARTING_HP - target.get_hp();
	}

	static constexpr int STARTING_HP = 100;

	Game game;
	GameContext ctx;
	std::unique_ptr<Player> player{ std::make_unique<Player>(Vector2D{ 4, 5 }) };
	Creature goblin{ Vector2D{ 5, 5 }, ActorData{ TileRef{}, "goblin", 0 } };
	Creature orc{ Vector2D{ 6, 5 }, ActorData{ TileRef{}, "orc", 0 } };
};

// The fixture's rolls mean what the tests say: 12 fails every attacker's save here and
// hits armour class 10, 20 makes the save, and 1 fails it.
TEST_F(SanctuarySaveTest, TheScriptedRollsMeanWhatTheTestsSay)
{
	const int monsterNeeds = SavingThrows::target(goblin.get_creature_class(), goblin.get_creature_level(), SavingThrow::SPELL);
	const int playerNeeds = SavingThrows::target(player->get_creature_class(), player->get_creature_level(), SavingThrow::SPELL);
	EXPECT_GT(monsterNeeds, 12);
	EXPECT_GT(playerNeeds, 12);
	EXPECT_LE(monsterNeeds, 20);
	EXPECT_LE(playerNeeds, 20);
}

// Failed on a 1, the goblin stays turned away; a second save would take the 20 and make it.
TEST_F(SanctuarySaveTest, AFailedSaveHoldsForTheCasting)
{
	cast_sanctuary(*player);
	script({ 1, 20 });

	EXPECT_TRUE(turned_away_from_player(goblin));
	EXPECT_TRUE(turned_away_from_player(goblin)) << "the goblin saved again against one casting";
}

// Made on a 20, the goblin is never turned away; a second save would take the 1 and fail.
TEST_F(SanctuarySaveTest, AMadeSaveHoldsForTheCasting)
{
	cast_sanctuary(*player);
	script({ 20, 1 });

	EXPECT_FALSE(turned_away_from_player(goblin));
	EXPECT_FALSE(turned_away_from_player(goblin)) << "the goblin saved again against one casting";
}

// Each attacker rolls its own save and keeps its own result.
TEST_F(SanctuarySaveTest, EachAttackerKeepsItsOwnSave)
{
	cast_sanctuary(*player);
	script({ 1, 20, 20, 1 });

	EXPECT_TRUE(turned_away_from_player(goblin));
	EXPECT_FALSE(turned_away_from_player(orc));
	EXPECT_TRUE(turned_away_from_player(goblin)) << "the goblin's failure was not kept";
	EXPECT_FALSE(turned_away_from_player(orc)) << "the orc's success was not kept";
}

// When the casting ends its record ends with it: the next casting is met with a fresh save.
TEST_F(SanctuarySaveTest, ANewCastingIsMetWithAFreshSave)
{
	cast_sanctuary(*player);
	script({ 1, 20 });
	ASSERT_TRUE(turned_away_from_player(goblin));

	ctx.buffSystem->remove_buff(*player, BuffType::SANCTUARY);
	cast_sanctuary(*player);

	EXPECT_FALSE(turned_away_from_player(goblin)) << "a failure against the last casting held against this one";
}

// A casting over a running one is still a new casting, and a save is good only against
// "that casting of the spell": the goblin that failed rolls again, and makes it.
TEST_F(SanctuarySaveTest, ACastingOverARunningOneIsMetWithAFreshSave)
{
	cast_sanctuary(*player);
	script({ 1, 20 });
	ASSERT_TRUE(turned_away_from_player(goblin));

	cast_sanctuary(*player);

	EXPECT_FALSE(turned_away_from_player(goblin)) << "a failure against the last casting held against this one";
}

// It cuts both ways: the goblin that made its save rolls again, and fails.
TEST_F(SanctuarySaveTest, ACastingOverARunningOneUndoesAMadeSave)
{
	cast_sanctuary(*player);
	script({ 20, 1 });
	ASSERT_FALSE(turned_away_from_player(goblin));

	cast_sanctuary(*player);

	EXPECT_TRUE(turned_away_from_player(goblin)) << "a success against the last casting held against this one";
}

// Another spell is not a new casting of this one: a bless leaves the saves standing.
TEST_F(SanctuarySaveTest, AnotherBuffLeavesTheSanctuaryRecordAlone)
{
	cast_sanctuary(*player);
	script({ 1, 20 });
	ASSERT_TRUE(turned_away_from_player(goblin));

	ctx.buffSystem->add_buff(*player, BuffType::BLESS, 1, 10, false);

	EXPECT_TRUE(turned_away_from_player(goblin)) << "a bless wiped the sanctuary's record";
}

// Without Sanctuary nobody is turned away, and the 1 queued for a save is never read.
TEST_F(SanctuarySaveTest, NoSanctuaryTurnsNobodyAway)
{
	script({ 1 });

	EXPECT_FALSE(turned_away_from_player(goblin));
}

// The record is part of the saved game: after a save and a load, the goblin that failed
// is still turned away, and the 20 queued for a fresh save is never read.
TEST_F(SanctuarySaveTest, TheRecordSurvivesASaveAndALoad)
{
	cast_sanctuary(*player);
	script({ 1 });
	ASSERT_TRUE(turned_away_from_player(goblin));

	nlohmann::json saved;
	player->save(saved);
	auto loaded = std::make_unique<Player>(Vector2D{ 0, 0 });
	loaded->healthPool = std::make_unique<HealthPool>(0);
	loaded->load(saved);
	player = std::move(loaded);
	script({ 20 });

	EXPECT_TRUE(turned_away_from_player(goblin)) << "the goblin's failed save was lost in the load";
}

// A monster attacking the warded player: the save's 12 fails, and a sure hit behind it
// would land if the attack went ahead.
TEST_F(SanctuarySaveTest, AWardedPlayerTurnsAMonstersAttackAway)
{
	cast_sanctuary(*player);
	script({ 12, 20, 4 });

	goblin.attacker->attack(*player, AttackKind::MELEE, ctx);

	EXPECT_EQ(hp_lost(*player), 0) << "the goblin struck through a failed save";
}

// A shot is held to the same save as a blow: the ranged monster's path asks it too.
TEST_F(SanctuarySaveTest, AWardedPlayerTurnsAMonstersShotAway)
{
	cast_sanctuary(*player);
	script({ 12, 20, 4 });

	goblin.attacker->attack(*player, AttackKind::RANGED, ctx);

	EXPECT_EQ(hp_lost(*player), 0) << "the goblin shot through a failed save";
}

// The warded player who attacks breaks their own Sanctuary: "the subject cannot take
// direct offensive action without breaking the spell".
TEST_F(SanctuarySaveTest, AttackingBreaksTheAttackersOwnSanctuary)
{
	cast_sanctuary(*player);
	script({ 20, 2 });

	player->attacker->attack(goblin, AttackKind::MELEE, ctx);

	EXPECT_FALSE(ctx.buffSystem->has_buff(*player, BuffType::SANCTUARY)) << "the player attacked and stayed warded";
}

// The player attacking a warded monster is held to the same save.
TEST_F(SanctuarySaveTest, AWardedMonsterTurnsThePlayersAttackAway)
{
	cast_sanctuary(goblin);
	script({ 12, 20, 2 });

	player->attacker->attack(goblin, AttackKind::MELEE, ctx);

	EXPECT_EQ(hp_lost(goblin), 0) << "the player struck a warded goblin through a failed save";
}

// So is one monster attacking another - a confused one, say.
TEST_F(SanctuarySaveTest, AWardedMonsterTurnsAnotherMonstersAttackAway)
{
	cast_sanctuary(goblin);
	script({ 12, 20, 4 });

	orc.attacker->attack(goblin, AttackKind::MELEE, ctx);

	EXPECT_EQ(hp_lost(goblin), 0) << "the orc struck a warded goblin through a failed save";
}

// A made save, and the attack goes ahead as usual: a hit, and the die shows four.
TEST_F(SanctuarySaveTest, AnAttackerThatSavesStrikesAsUsual)
{
	cast_sanctuary(goblin);
	script({ 20, 20, 4 });

	orc.attacker->attack(goblin, AttackKind::MELEE, ctx);

	EXPECT_EQ(hp_lost(goblin), 4) << "1d4 at four after a made save";
}

// An unwarded target costs no save: the first roll is the attack.
TEST_F(SanctuarySaveTest, AnUnwardedTargetCostsNoSave)
{
	script({ 20, 4 });

	orc.attacker->attack(goblin, AttackKind::MELEE, ctx);

	EXPECT_EQ(hp_lost(goblin), 4) << "a save was rolled against a target without sanctuary";
}
