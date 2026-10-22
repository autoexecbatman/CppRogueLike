// Checks that a web spinner's bite obeys Sanctuary as every other monster's attack does.
//
// What it is for. The Player's Handbook, PDF page 436 of the 2e archive: "any opponent
// attempting to strike or otherwise directly attack the protected creature must roll a
// saving throw vs. spell". The save is made where every attack resolves, so a web
// spinner's bite and its venom are held off by it like any other attack.
//
// Every roll is scripted in the order the game asks for it: the sanctuary save if the
// player is protected, then the attack roll, the damage die and the player's save
// against the venom, queued at the top of the die so no venom lands on the bite.
//
// WebSpinnerQuarryTest checks the other half of the spell: a spinner that failed its save
// "totally ignores the warded creature", so it neither bites nor aims a web at it. Laying
// a web is an area effect, which the spell does not stop, so no save is rolled to spin.
//
// Run it, from build/bin/Debug:
//
//     cmake --build build --config Debug --target test_exe
//     ./test_exe.exe --gtest_filter=WebSpinnerSanctuaryTest.*:WebSpinnerQuarryTest.*

#include <gtest/gtest.h>

#include <algorithm>
#include <initializer_list>
#include <memory>
#include <ranges>

#include "src/Actor.h"
#include "src/AiWebSpinner.h"
#include "src/ArmorClass.h"
#include "src/BuffSystem.h"
#include "src/BuffType.h"
#include "src/Colors.h"
#include "src/Creature.h"
#include "src/DamageInfo.h"
#include "src/ExperienceReward.h"
#include "src/Game.h"
#include "src/HealthPool.h"
#include "src/Map.h"
#include "src/MonsterAttacker.h"
#include "src/Paths.h"
#include "src/Player.h"
#include "src/RandomDice.h"
#include "src/SavingThrow.h"
#include "src/SpellTile.h"
#include "src/Web.h"
#include "tests/mocks/TestDice.h"

// The test a spell tile passes when it is a standing web outside the columns
// westmostCol to eastmostCol.
//
// Example, with a web standing at column 9:
//   std::ranges::any_of(*ctx.spellTiles, standing_outside(10, 14)); // -> true
auto standing_outside(int westmostCol, int eastmostCol)
{
	return [westmostCol, eastmostCol](const std::unique_ptr<SpellTile>& tile)
	{
		const bool isOutside = tile->position.x < westmostCol || tile->position.x > eastmostCol;
		return !tile->is_destroyed() && isOutside;
	};
}

class WebSpinnerSanctuaryTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		game.dataManager.load_all_data(game.messageSystem);

		player->experienceReward = std::make_unique<ExperienceReward>(0);
		player->healthPool = std::make_unique<HealthPool>(STARTING_HP);
		player->armorClass = std::make_unique<ArmorClass>(10);
		player->set_dr(0);
		player->set_strength(10);
		player->set_dexterity(10);

		ctx = game.context();
		ctx.playerOwner = &player;
		ctx.map = &map;
		game.dice.set_test_mode(true);

		// An open corridor, so the spinner beside the player is in view.
		map.init_tiles();
		for (int col = 1; col < 19; ++col)
		{
			map.set_tile(Vector2D{ col, 5 }, TileType::FLOOR);
		}
		map.compute_fov(ctx);

		// A 1d4 bite from THAC0 20; strength 8 adds nothing to the damage.
		spinner.experienceReward = std::make_unique<ExperienceReward>(0);
		spinner.healthPool = std::make_unique<HealthPool>(10);
		spinner.armorClass = std::make_unique<ArmorClass>(5);
		spinner.attacker = std::make_unique<MonsterAttacker>(spinner, DamageInfo{ "1d4", DamageType::PHYSICAL });
		spinner.set_dr(0);
		spinner.set_thaco(20);
		spinner.set_strength(8);
		spinner.set_dexterity(10);
		spinner.set_natural_attack("fangs");
		spinner.ai = std::make_unique<AiWebSpinner>();
	}

	void TearDown() override
	{
		game.dice.set_test_mode(false);
		game.dice.clear_fixed_rolls();
	}

	// Queues rolls in the order the game asks for them.
	void script(std::initializer_list<int> rolls)
	{
		for (const int roll : rolls)
		{
			game.dice.set_next_roll(roll);
		}
	}

	int hp_lost_to_one_turn()
	{
		spinner.ai->update(spinner, ctx);
		return STARTING_HP - player->get_hp();
	}

	static constexpr int STARTING_HP = 100;

	Game game{ TEST_DICE_SEED };
	GameContext ctx;
	Map map{ 20, 20 };
	std::unique_ptr<Player> player{ std::make_unique<Player>(Vector2D{ 4, 5 }) };
	Creature spinner{ Vector2D{ 5, 5 }, ActorData{ TileRef{}, "web weaver", ColorPairId::WHITE_BLACK } };
};

// The spinner saves against the spell on a 12 and fails. A hit is queued behind it, so
// a spinner that skipped the save, or rolled it and bit anyway, lands damage.
TEST_F(WebSpinnerSanctuaryTest, SanctuaryHoldsOffASpinnerThatFailsToSave)
{
	ASSERT_GT(SavingThrows::target(spinner.get_creature_class(), spinner.get_creature_level(), SavingThrow::SPELL), 12);
	ctx.buffSystem->add_buff(*player, BuffType::SANCTUARY, 0, 10, BuffApplication::ADD, ctx.gameState->get_time());
	// The save, the attack, the bite's 1d4, then the player's own save against the
	// venom, which the highest roll makes - so no venom lands either way.
	script({ 12, 20, 4, RandomDice::HIGHEST });

	EXPECT_EQ(hp_lost_to_one_turn(), 0) << "sanctuary held and the spinner bit anyway";
}

// A 20 makes the save, and the spinner bites as usual: a hit, and the die shows four.
// Without the save, the attack would take the 20 and the die the next 20.
TEST_F(WebSpinnerSanctuaryTest, ASpinnerThatSavesBitesAsUsual)
{
	ctx.buffSystem->add_buff(*player, BuffType::SANCTUARY, 0, 10, BuffApplication::ADD, ctx.gameState->get_time());
	script({ 20, 20, 4, RandomDice::HIGHEST }); // the last is the player's venom save, made

	EXPECT_EQ(hp_lost_to_one_turn(), 4) << "1d4 at four after a made save";
}

// Venom is a direct attack too. A spinner certain to inject it fails its save on a 12:
// the bite is turned away, the venom roll of 20 comes in under its chance of 100, and
// a 3 waits for the venom's damage, which must never be read.
TEST_F(WebSpinnerSanctuaryTest, NoVenomFromASpinnerTurnedAway)
{
	spinner.ai = std::make_unique<AiWebSpinner>();
	ctx.buffSystem->add_buff(*player, BuffType::SANCTUARY, 0, 10, BuffApplication::ADD, ctx.gameState->get_time());
	script({ 12, 20, 3 });

	EXPECT_EQ(hp_lost_to_one_turn(), 0) << "venom landed through a failed save";
}

// No sanctuary, no save: the first roll is the attack.
TEST_F(WebSpinnerSanctuaryTest, AnUnwardedPlayerIsBittenWithoutASave)
{
	script({ 20, 4, RandomDice::HIGHEST }); // the attack, the 1d4, then the venom save, made

	EXPECT_EQ(hp_lost_to_one_turn(), 4) << "a save was rolled against a player without sanctuary";
}

// The spinning tests below stand the spinner four tiles off, at (8,5), the edge of the
// player's sight radius, so in view. Only row 5 is floor, and a CIRCULAR web (pattern 0) always lays its two edge
// tiles on that row, size tiles either side of its centre, so where the edges land names
// the centre and size the spinner chose.
class WebSpinnerQuarryTest : public WebSpinnerSanctuaryTest
{
protected:
	void SetUp() override
	{
		WebSpinnerSanctuaryTest::SetUp();
		// A web takes its tile from the config, so a spun web needs it loaded.
		game.tileConfig.load(Paths::TILE_CONFIG);
		spinner.position = Vector2D{ 8, 5 };
	}

	void ward_player()
	{
		ctx.buffSystem->add_buff(*player, BuffType::SANCTUARY, 0, 10, BuffApplication::ADD, ctx.gameState->get_time());
	}

	// The spinner's save against the ward, rolled and recorded as an attack would.
	void record_spinner_save(int roll)
	{
		script({ roll });
		ctx.buffSystem->is_turned_away_by_sanctuary(spinner, *player, ctx);
	}

	bool has_web_at(Vector2D position) const
	{
		auto stands_at = [position](const std::unique_ptr<SpellTile>& spellTile)
		{
			return !spellTile->is_destroyed() && spellTile->position == position;
		};
		return std::ranges::any_of(*ctx.spellTiles, stands_at);
	}

	bool has_web_outside(int westmostCol, int eastmostCol) const
	{
		auto stands_outside = standing_outside(westmostCol, eastmostCol);
		return std::ranges::any_of(*ctx.spellTiles, stands_outside);
	}

	// Five webs at the corridor's east end, the most a spinner keeps.
	void stand_five_webs()
	{
		for (const int col : std::views::iota(14, 19))
		{
			ctx.spellTiles->emplace_back(std::make_unique<Web>(Vector2D{ col, 5 }, 3, *ctx.tileConfig));
		}
	}
};

// A spinner that failed its save has lost track of the player, so it spins as though
// nobody were there: the 10% trap web, minimum size 3, on itself - edges at (5,5) and
// (11,5). Aimed at the player it would centre on (6,5) at size 5 and reach (1,5).
TEST_F(WebSpinnerQuarryTest, FailedSpinnerSpinsATrapWebOnItself)
{
	ASSERT_GT(SavingThrows::target(spinner.get_creature_class(), spinner.get_creature_level(), SavingThrow::SPELL), 12);
	ward_player();
	record_spinner_save(12);
	ASSERT_TRUE(ctx.buffSystem->ignores_warded_creature(spinner, *player));
	// The d100 of 1 is the trap web's 10%, then pattern 0, CIRCULAR.
	script({ 1, 0 });

	spinner.ai->update(spinner, ctx);

	EXPECT_TRUE(has_web_at(Vector2D{ 5, 5 })) << "no web edge three tiles west of the spinner";
	EXPECT_TRUE(has_web_at(Vector2D{ 11, 5 })) << "no web edge three tiles east of the spinner";
	EXPECT_FALSE(has_web_outside(5, 11)) << "the web was aimed at, or sized for, a player it has lost";
}

// A made save leaves the player in plain sight: the 40% defensive web, size 5 for a player
// under five tiles off, centred two tiles toward the player at (6,5), so an edge at (1,5).
TEST_F(WebSpinnerQuarryTest, SavedSpinnerAimsAtThePlayerAsBefore)
{
	ward_player();
	record_spinner_save(20);
	ASSERT_FALSE(ctx.buffSystem->ignores_warded_creature(spinner, *player));
	script({ 1, 0 });

	spinner.ai->update(spinner, ctx);

	EXPECT_TRUE(has_web_at(Vector2D{ 1, 5 })) << "a spinner that saved did not aim at the player";
}

// No Sanctuary: the same aimed web.
TEST_F(WebSpinnerQuarryTest, UnwardedSpinnerAimsAtThePlayerAsBefore)
{
	script({ 1, 0 });

	spinner.ai->update(spinner, ctx);

	EXPECT_TRUE(has_web_at(Vector2D{ 1, 5 })) << "an unwarded player was not aimed at";
}

// Spinning is no attack, so deciding to spin rolls no save against the ward: the casting
// holds no record for the spinner afterwards. A save rolled here would take the queued 1.
TEST_F(WebSpinnerQuarryTest, DecidingToSpinRollsNoSave)
{
	ward_player();
	script({ 1, 0 });

	spinner.ai->update(spinner, ctx);

	ASSERT_EQ(player->activeBuffs.size(), 1u);
	EXPECT_TRUE(player->activeBuffs.front().opponentSaves.empty()) << "a save was rolled to decide on a web";
}

// Beside the player, a spinner that has lost track of it does not try to bite and get
// turned away. It takes the trap web instead, centred on itself at (5,5), whose east
// edge is (8,5); a bite would end the turn with no web at all.
TEST_F(WebSpinnerQuarryTest, FailedSpinnerBesideThePlayerSpinsInsteadOfBiting)
{
	spinner.position = Vector2D{ 5, 5 };
	ward_player();
	record_spinner_save(12);
	ASSERT_TRUE(ctx.buffSystem->ignores_warded_creature(spinner, *player));
	script({ 1, 0 });

	EXPECT_EQ(hp_lost_to_one_turn(), 0);
	EXPECT_TRUE(has_web_at(Vector2D{ 8, 5 })) << "the spinner bit at a player it has lost instead of spinning";
}

// With five webs standing, only a quarry's 40% draws another. A d100 of 30 is under it
// but over the trap web's 10%, so a spinner that has lost the player spins nothing.
TEST_F(WebSpinnerQuarryTest, FailedSpinnerAtItsWebLimitSpinsNoDefensiveWeb)
{
	stand_five_webs();
	ward_player();
	record_spinner_save(12);
	script({ 30 });

	spinner.ai->update(spinner, ctx);

	EXPECT_EQ(ctx.spellTiles->size(), 5u) << "a defensive web was spun against a player it has lost";
}

// The control for the test above: the same 30, with the player in sight, spins a web.
TEST_F(WebSpinnerQuarryTest, UnwardedSpinnerAtItsWebLimitSpinsADefensiveWeb)
{
	stand_five_webs();
	script({ 30, 0 });

	spinner.ai->update(spinner, ctx);

	EXPECT_GT(ctx.spellTiles->size(), 5u) << "the 40% defensive web was not spun";
}
