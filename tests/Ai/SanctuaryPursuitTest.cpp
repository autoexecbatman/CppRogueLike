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
#include "src/Creature.h"
#include "src/DamageInfo.h"
#include "src/ExperienceReward.h"
#include "src/Game.h"
#include "src/HealthPool.h"
#include "src/Map.h"
#include "src/MessageSystem.h"
#include "src/MonsterAttacker.h"
#include "src/Player.h"
#include "src/PlayerAttacker.h"

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
	void make_the_goblin_fail_its_save()
	{
		script({ 12, 20, 4 });
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
	Creature goblin{ Vector2D{ GOBLIN_START_COLUMN, CORRIDOR_ROW }, ActorData{ TileRef{}, "goblin", 0 } };
};

TEST_F(SanctuaryPursuitTest, AMonsterThatFailedItsSaveStopsClosingIn)
{
	ctx.buffSystem->add_buff(*player, BuffType::SANCTUARY, 0, 10, false);
	make_the_goblin_fail_its_save();
	const int distanceBefore = goblin_distance_to_player();

	// Whatever the monster does instead of hunting reads dice: a zero is not the 1 that
	// starts a wander, and a zero step is no step, so nothing here moves it by accident.
	script({ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 });
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
	ctx.buffSystem->add_buff(*player, BuffType::SANCTUARY, 0, 10, false);
	script({ 20, 20, 4 });
	goblin.attacker->attack(*player, AttackKind::MELEE, ctx);
	const int distanceBefore = goblin_distance_to_player();

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
	ctx.buffSystem->add_buff(*player, BuffType::SANCTUARY, 0, 10, false);
	const int distanceBefore = goblin_distance_to_player();

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
	ctx.buffSystem->add_buff(*player, BuffType::SANCTUARY, 0, 10, false);
	make_the_goblin_fail_its_save();
	const size_t messagesAfterTheOneRefusal = game.messageSystem.get_stored_message_count();

	script({ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 });
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
	ctx.buffSystem->add_buff(*player, BuffType::SANCTUARY, 0, 10, false);
	make_the_goblin_fail_its_save();
	const size_t messagesAfterTheOneRefusal = game.messageSystem.get_stored_message_count();

	script({ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 });
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
	goblin.ai = std::make_unique<AiWebSpinner>(0);
	ctx.buffSystem->add_buff(*player, BuffType::SANCTUARY, 0, 10, false);
	make_the_goblin_fail_its_save();
	const int distanceBefore = goblin_distance_to_player();

	script({ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 });
	for (int round = 0; round < 4; ++round)
	{
		goblin.ai->update(goblin, ctx);
	}

	EXPECT_GE(goblin_distance_to_player(), distanceBefore)
		<< "the spinner walked toward a creature it had lost track of";
}
