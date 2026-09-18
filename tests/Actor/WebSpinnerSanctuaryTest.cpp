// Checks that a web spinner's bite obeys Sanctuary as every other monster's attack does.
//
// What it is for. The Player's Handbook, PDF page 436 of the 2e archive: "any opponent
// attempting to strike or otherwise directly attack the protected creature must roll a
// saving throw vs. spell". The save is made where every attack resolves, so a web
// spinner's bite and its venom are held off by it like any other attack.
//
// Every roll is scripted in the order the game asks for it: the sanctuary save if the
// player is protected, then the attack roll, the damage die and the venom roll. The
// spider's poison chance is zero, so no venom lands on top of the bite.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=WebSpinnerSanctuaryTest.*

#include <gtest/gtest.h>

#include <initializer_list>
#include <memory>

#include "src/Actor.h"
#include "src/AiWebSpinner.h"
#include "src/ArmorClass.h"
#include "src/BuffSystem.h"
#include "src/BuffType.h"
#include "src/Creature.h"
#include "src/DamageInfo.h"
#include "src/ExperienceReward.h"
#include "src/Game.h"
#include "src/HealthPool.h"
#include "src/Map.h"
#include "src/MonsterAttacker.h"
#include "src/Player.h"
#include "src/SavingThrow.h"

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
			map.set_tile(Vector2D{ col, 5 }, TileType::FLOOR, 1.0);
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
		spinner.ai = std::make_unique<AiWebSpinner>(0);
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

	Game game;
	GameContext ctx;
	Map map{ 20, 20 };
	std::unique_ptr<Player> player{ std::make_unique<Player>(Vector2D{ 4, 5 }) };
	Creature spinner{ Vector2D{ 5, 5 }, ActorData{ TileRef{}, "web weaver", 0 } };
};

// The spinner saves against the spell on a 12 and fails. A hit is queued behind it, so
// a spinner that skipped the save, or rolled it and bit anyway, lands damage.
TEST_F(WebSpinnerSanctuaryTest, SanctuaryHoldsOffASpinnerThatFailsToSave)
{
	ASSERT_GT(SavingThrows::target(spinner.get_creature_class(), spinner.get_creature_level(), SavingThrow::SPELL), 12);
	ctx.buffSystem->add_buff(*player, BuffType::SANCTUARY, 0, 10, false);
	script({ 12, 20, 4, 100 });

	EXPECT_EQ(hp_lost_to_one_turn(), 0) << "sanctuary held and the spinner bit anyway";
}

// A 20 makes the save, and the spinner bites as usual: a hit, and the die shows four.
// Without the save, the attack would take the 20 and the die the next 20.
TEST_F(WebSpinnerSanctuaryTest, ASpinnerThatSavesBitesAsUsual)
{
	ctx.buffSystem->add_buff(*player, BuffType::SANCTUARY, 0, 10, false);
	script({ 20, 20, 4, 100 });

	EXPECT_EQ(hp_lost_to_one_turn(), 4) << "1d4 at four after a made save";
}

// Venom is a direct attack too. A spinner certain to inject it fails its save on a 12:
// the bite is turned away, the venom roll of 20 comes in under its chance of 100, and
// a 3 waits for the venom's damage, which must never be read.
TEST_F(WebSpinnerSanctuaryTest, NoVenomFromASpinnerTurnedAway)
{
	spinner.ai = std::make_unique<AiWebSpinner>(100);
	ctx.buffSystem->add_buff(*player, BuffType::SANCTUARY, 0, 10, false);
	script({ 12, 20, 3 });

	EXPECT_EQ(hp_lost_to_one_turn(), 0) << "venom landed through a failed save";
}

// No sanctuary, no save: the first roll is the attack.
TEST_F(WebSpinnerSanctuaryTest, AnUnwardedPlayerIsBittenWithoutASave)
{
	script({ 20, 4, 100 });

	EXPECT_EQ(hp_lost_to_one_turn(), 4) << "a save was rolled against a player without sanctuary";
}
