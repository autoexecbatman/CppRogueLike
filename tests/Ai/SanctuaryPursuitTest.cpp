// file: SanctuaryPursuitTest.cpp
// A monster that failed its save against Sanctuary stops hunting the warded creature.
//
// What it is for. The Player's Handbook, PDF page 436 of the 2e archive: a failed save
// means "the opponent loses track of and totally ignores the warded creature for the
// duration of the spell". The attack was already turned away, but the monster kept
// walking up to the player and trying again every round, which is neither losing track
// of them nor ignoring them.
//
// The roll stays where "attempting to strike" happens, at the attack. The AI reads the
// recorded result and never rolls, because the same page says "those not attempting to
// attack the subject remain unaffected" - a monster that has not attacked has not saved.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=SanctuaryPursuitTest.*

#include <gtest/gtest.h>

#include <initializer_list>
#include <memory>

#include "src/AiMonster.h"
#include "src/AiMonsterRanged.h"
#include "src/AiWebSpinner.h"
#include "src/ArmorClass.h"
#include "src/AttackKind.h"
#include "src/BuffSystem.h"
#include "src/BuffType.h"
#include "src/Colors.h"
#include "src/Creature.h"
#include "src/DamageInfo.h"
#include "src/ExperienceReward.h"
#include "src/Game.h"
#include "src/HealthPool.h"
#include "src/Map.h"
#include "src/MessageSystem.h"
#include "src/MonsterAttacker.h"
#include "src/Paths.h"
#include "src/Player.h"
#include "src/PlayerAttacker.h"
#include "src/RandomDice.h"

namespace
{

constexpr int STARTING_HP = 100;

// Inside the player's field of view, so update_awareness makes the goblin aware on its
// own turn, and far enough that three steps of closing in are unmistakable.
constexpr int GOBLIN_START_COLUMN = 8;
constexpr int PLAYER_COLUMN = 4;
constexpr int CORRIDOR_ROW = 5;

} // namespace

class SanctuaryPursuitTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		game.dataManager.load_all_data(game.messageSystem);
		// The web spinner's own update can lay a web, which needs TILE_WEB. Without
		// this the spinner case passed only because an out-of-range scripted roll
		// stopped the web being built at all.
		game.tileConfig.load(Paths::TILE_CONFIG);

		player->experienceReward = std::make_unique<ExperienceReward>(0);
		player->healthPool = std::make_unique<HealthPool>(STARTING_HP);
		player->armorClass = std::make_unique<ArmorClass>(10);
		player->attacker = std::make_unique<PlayerAttacker>(*player);
		player->set_dr(0);
		player->set_thaco(20);
		player->set_strength(10);
		player->set_dexterity(10);

		goblin.experienceReward = std::make_unique<ExperienceReward>(0);
		goblin.healthPool = std::make_unique<HealthPool>(STARTING_HP);
		goblin.armorClass = std::make_unique<ArmorClass>(10);
		goblin.attacker = std::make_unique<MonsterAttacker>(goblin, DamageInfo{ "1d4", DamageType::PHYSICAL });
		goblin.set_dr(0);
		goblin.set_thaco(20);
		goblin.set_strength(8);
		goblin.set_dexterity(10);
		goblin.set_natural_attack("bite");
		goblin.ai = std::make_unique<AiMonster>();

		ctx = game.context();
		ctx.playerOwner = &player;
		ctx.map = &map;
		ctx.creatures = &creatures;

		// One open corridor, so the only way to travel is along it and any change in
		// distance is the monster deciding to close.
		map.init_tiles();
		for (int column = 1; column < 19; ++column)
		{
			map.set_tile(Vector2D{ column, CORRIDOR_ROW }, TileType::FLOOR, 1.0);
		}
		map.compute_fov(ctx);
		map.rebuild_dijkstra_map({ player->position }, ctx);

		game.dice.set_test_mode(true);
	}

	void TearDown() override
	{
		game.dice.set_test_mode(false);
		game.dice.clear_fixed_rolls();
	}

	// Queues "top of the die" rolls to cover the idle rounds that follow. Idling rolls
	// a d20 and acts only on a 1, so the top of the die never starts one.
	//
	// The count has to cover every roll the rounds make, not the rounds themselves:
	// when the queue runs dry RandomDice falls back to real dice, and the test then
	// depends on them. Measured 2026-10-04, the web spinner's four rounds asked for 58
	// rolls against 22 queued, and the 36 real ones decided whether it span a web - the
	// one flake in an otherwise green suite. The counts below are measured with room
	// over the top, and a run that reaches the end of the queue is a run to re-measure.
	void script_idle_rolls(int count)
	{
		for (int queued = 0; queued < count; ++queued)
		{
			game.dice.set_next_roll(RandomDice::HIGHEST);
		}
	}

	void script(std::initializer_list<int> rolls)
	{
		for (const int roll : rolls)
		{
			game.dice.set_next_roll(roll);
		}
	}

	// Rolls the goblin's save against the casting by having it attack once, and reports
	// what the record now says. 12 fails the save here; the queued rolls behind it would
	// land a hit if the save were skipped.
	// The save it fails, and whatever else the refused swing reads. That is not the
	// same sequence for every creature: the goblin reads two more d20s, while the web
	// spinner reads a d100 for whether it spins a web and then a pattern. So the caller
	// scripts its own rolls instead of sharing values that land on different dice.
	void fail_the_save_with(std::initializer_list<int> rolls)
	{
		script(rolls);
		goblin.attacker->attack(*player, AttackKind::MELEE, ctx);
		ASSERT_EQ(player->get_hp(), STARTING_HP) << "the save was skipped and the blow landed";
	}

	int goblin_distance_to_player() const
	{
		return goblin.get_tile_distance(player->position);
	}

	Game game;
	GameContext ctx;
	Map map{ 20, 20 };
	std::vector<std::unique_ptr<Creature>> creatures{};
	std::unique_ptr<Player> player{ std::make_unique<Player>(Vector2D{ PLAYER_COLUMN, CORRIDOR_ROW }) };
	Creature goblin{ Vector2D{ GOBLIN_START_COLUMN, CORRIDOR_ROW }, ActorData{ TileRef{}, "goblin", ColorPairId::WHITE_BLACK } };
};

TEST_F(SanctuaryPursuitTest, AMonsterThatFailedItsSaveStopsClosingIn)
{
	ctx.buffSystem->add_buff(*player, BuffType::SANCTUARY, 0, 10, false, ctx.gameState->get_time());
	fail_the_save_with({ 12, 20, 4 });
	const int distanceBefore = goblin_distance_to_player();

	// Whatever the monster does instead of hunting reads dice. Idling rolls a d20 and
	// acts only on a 1, so the top of the die is never that 1 - which a zero also was
	// not, except a zero is a roll no d20 can make.
	script_idle_rolls(120);
	for (int round = 0; round < 4; ++round)
	{
		goblin.ai->update(goblin, ctx);
	}

	EXPECT_GE(goblin_distance_to_player(), distanceBefore)
		<< "the goblin walked toward a creature it had lost track of";
}

TEST_F(SanctuaryPursuitTest, AMonsterThatMadeItsSaveKeepsHunting)
{
	// The other half of the rule: a made save leaves the opponent unaffected, so it must
	// still come. Without this the fix could be "no monster ever pursues" and pass.
	ctx.buffSystem->add_buff(*player, BuffType::SANCTUARY, 0, 10, false, ctx.gameState->get_time());
	script({ 20, 20, 4 });
	goblin.attacker->attack(*player, AttackKind::MELEE, ctx);
	const int distanceBefore = goblin_distance_to_player();

	script_idle_rolls(120);
	for (int round = 0; round < 4; ++round)
	{
		goblin.ai->update(goblin, ctx);
	}

	EXPECT_LT(goblin_distance_to_player(), distanceBefore)
		<< "a goblin that made its save lost interest anyway";
}

TEST_F(SanctuaryPursuitTest, AMonsterThatNeverAttackedIsNotTurnedAway)
{
	// "Those not attempting to attack the subject remain unaffected": reading the record
	// must not roll a save, so a monster that has never swung still hunts.
	ctx.buffSystem->add_buff(*player, BuffType::SANCTUARY, 0, 10, false, ctx.gameState->get_time());
	const int distanceBefore = goblin_distance_to_player();

	script_idle_rolls(120);
	for (int round = 0; round < 4; ++round)
	{
		goblin.ai->update(goblin, ctx);
	}

	EXPECT_LT(goblin_distance_to_player(), distanceBefore)
		<< "a goblin that never attacked was turned away without rolling";
}

TEST_F(SanctuaryPursuitTest, AMonsterThatLostTrackStopsAnnouncingItEveryRound)
{
	// The symptom a player sees: a monster that has lost track of the warded creature
	// used to walk up and swing anyway, and every round the log said so - Attacker prints
	// "cannot bring itself to attack" whenever a turned-away swing is made. Once it stops
	// hunting there is no swing to turn away, so the line stops too.
	ctx.buffSystem->add_buff(*player, BuffType::SANCTUARY, 0, 10, false, ctx.gameState->get_time());
	fail_the_save_with({ 12, 20, 4 });
	const size_t messagesAfterTheOneRefusal = game.messageSystem.get_stored_message_count();

	script_idle_rolls(120);
	for (int round = 0; round < 4; ++round)
	{
		goblin.ai->update(goblin, ctx);
	}

	EXPECT_EQ(game.messageSystem.get_stored_message_count(), messagesAfterTheOneRefusal)
		<< "the goblin kept swinging at a creature it had lost track of, once a round";
}

TEST_F(SanctuaryPursuitTest, ARangedMonsterThatLostTrackStopsShootingToo)
{
	// The same rule reaches every opponent, so the archer's own update is gated as well.
	// At this distance it shoots rather than closes, so the shot is what must stop.
	goblin.ai = std::make_unique<AiMonsterRanged>();
	ctx.buffSystem->add_buff(*player, BuffType::SANCTUARY, 0, 10, false, ctx.gameState->get_time());
	fail_the_save_with({ 12, 20, 4 });
	const size_t messagesAfterTheOneRefusal = game.messageSystem.get_stored_message_count();

	script_idle_rolls(120);
	for (int round = 0; round < 4; ++round)
	{
		goblin.ai->update(goblin, ctx);
	}

	EXPECT_EQ(game.messageSystem.get_stored_message_count(), messagesAfterTheOneRefusal)
		<< "the archer kept loosing arrows at a creature it had lost track of";
}

TEST_F(SanctuaryPursuitTest, AWebSpinnerThatLostTrackStopsClosingIn)
{
	// The spinner has its own update and so its own gate. Its poison chance is zero, so
	// nothing here depends on venom - only on whether it still walks toward the player.
	goblin.ai = std::make_unique<AiWebSpinner>();
	ctx.buffSystem->add_buff(*player, BuffType::SANCTUARY, 0, 10, false, ctx.gameState->get_time());
	// The spinner's second roll is the d100 for whether it spins a web, and spinning
	// needs a tile this fixture does not load. The top of the die is over the chance.
	fail_the_save_with({ 12, RandomDice::HIGHEST });
	const int distanceBefore = goblin_distance_to_player();

	script_idle_rolls(120);
	for (int round = 0; round < 4; ++round)
	{
		goblin.ai->update(goblin, ctx);
	}

	EXPECT_GE(goblin_distance_to_player(), distanceBefore)
		<< "the spinner walked toward a creature it had lost track of";
}
