// file: PotionOfSpeedTest.cpp
// The potion of speed doubles its drinker's movement and attacks for 5d4 rounds, does not
// speed up spellcasting, and ages its drinker by a year.
//
// - DMG, potion of speed (PDF page 895): "increases the movement and combat capabilities
//   of the imbiber by 100%. [...] This does not reduce spellcasting time, however. Use of a
//   speed potion ages the individual by one year. [...] the other effects last for 5d4
//   rounds."
// - PHB, Haste (PDF page 308): "This spell is not cumulative with itself or with other
//   similar magic."
//
// Doubling is the action delay halved while a SPEED buff is present, so the effect ends
// when the buff leaves activeBuffs, with nothing to undo. The cast-menu case runs in a
// child process with tests/mocks/HiddenRenderer.h, and skips where there is no OpenGL
// driver.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=PotionOfSpeed*

#include <gtest/gtest.h>

#include <cstdlib>
#include <memory>
#include <string_view>
#include <utility>
#include <variant>

#include <nlohmann/json.hpp>

#include "src/ArmorClass.h"
#include "src/BuffSystem.h"
#include "src/BuffType.h"
#include "src/Colors.h"
#include "src/Controls.h"
#include "src/Creature.h"
#include "src/EquipmentSlot.h"
#include "src/ExperienceReward.h"
#include "src/GameContext.h"
#include "src/HealthPool.h"
#include "src/InputSystem.h"
#include "src/InventoryOperations.h"
#include "src/Item.h"
#include "src/ItemCreator.h"
#include "src/Lifespan.h"
#include "src/MenuManager.h"
#include "src/MenuSpellCast.h"
#include "src/Pickable.h"
#include "src/Player.h"
#include "src/PlayerRaceState.h"
#include "src/PlayerTurn.h"
#include "src/RandomDice.h"
#include "src/Renderer.h"
#include "src/TurnSchedule.h"
#include "src/Vector2D.h"
#include "tests/mocks/HiddenRenderer.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
// One ordinary action, and the hasted one: twice the actions is half the time each.
constexpr int ORDINARY_DELAY = TIME_UNITS_PER_ROUND;
constexpr int HASTED_DELAY = TIME_UNITS_PER_ROUND / 2;
// A moment well into a game, so a cost measured from time zero cannot pass by accident.
constexpr int TIME_OF_DRINKING = 5000;
// The four dice of 5d4 at their least and their most: 5 and 20 rounds.
constexpr int LOWEST_D4 = 1;
constexpr int HIGHEST_D4 = 4;
constexpr int DICE_IN_THE_DURATION = 5;

// The player, wired into a context, with nothing worn and nothing drunk.
void make_player(MockGameContext& mock, GameContext& ctx)
{
	mock.player = std::make_unique<Player>(Vector2D{ 2, 2 });
	Player& player = *mock.player;
	player.experienceReward = std::make_unique<ExperienceReward>(0);
	player.armorClass = std::make_unique<ArmorClass>(10);
	player.healthPool = std::make_unique<HealthPool>(20);
	player.set_strength(10);
	player.set_dexterity(10);
	ctx.playerOwner = &mock.player;
}

// A SPEED buff of the potion's kind, ending at the given clock reading.
Buff speed_ending_at(int expiryTime)
{
	Buff speed{};
	speed.type = BuffType::SPEED;
	speed.expiryTime = expiryTime;
	return speed;
}
} // namespace

class PotionOfSpeedTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		make_player(mock, ctx);
		ctx.gameState->set_time(TIME_OF_DRINKING);
	}

	Player& player() { return *mock.player; }

	// Drinks the potion of speed from the pack, with every die of its duration scripted to `rolled`.
	void drink_with_every_die_at(int rolled)
	{
		auto created = ItemCreator::create("potion_of_speed", player().position, ctx);
		Item& potion = *created;
		[[maybe_unused]] const auto added = InventoryOperations::add_item(player().inventoryData, std::move(created));
		ASSERT_TRUE(added.has_value()) << "the potion did not fit in the pack";
		for (int die = 0; die < DICE_IN_THE_DURATION; ++die)
		{
			mock.dice.set_next_roll(rolled);
		}
		use_item(*potion.behavior, potion, player(), ctx);
	}

	// Whether the drinker is still hasted once the clock reads `rounds` of game time past the drink.
	bool hasted_after(int rounds)
	{
		ctx.buffSystem->update_creature_buffs(player(), expiry_time(TIME_OF_DRINKING, rounds));
		return ctx.buffSystem->has_buff(player(), BuffType::SPEED);
	}

	MockGameContext mock{};
	GameContext ctx{ mock.to_game_context() };
};

TEST_F(PotionOfSpeedTest, ACreatureWithoutSpeedActsAtItsBaseDelay)
{
	EXPECT_EQ(player().get_action_delay(), ORDINARY_DELAY);
	EXPECT_EQ(player().get_unhasted_action_delay(), ORDINARY_DELAY);
}

TEST_F(PotionOfSpeedTest, AHastedCreatureActsInHalfItsDelay)
{
	ctx.buffSystem->add_buff(player(), BuffType::SPEED, 0, 10, BuffApplication::ADD, TIME_OF_DRINKING);

	EXPECT_EQ(player().get_action_delay(), HASTED_DELAY);
	EXPECT_EQ(player().get_unhasted_action_delay(), ORDINARY_DELAY) << "the base delay itself was changed";
}

// The halving reads the buff, so its expiry is the whole of the ending: nothing restores the delay.
TEST_F(PotionOfSpeedTest, TheDelayReturnsWhenSpeedRunsOut)
{
	ctx.buffSystem->add_buff(player(), BuffType::SPEED, 0, 3, BuffApplication::ADD, TIME_OF_DRINKING);
	ASSERT_EQ(player().get_action_delay(), HASTED_DELAY);

	ctx.buffSystem->update_creature_buffs(player(), expiry_time(TIME_OF_DRINKING, 3));

	EXPECT_EQ(player().get_action_delay(), ORDINARY_DELAY);
}

// "not cumulative with itself": two castings at once still halve the delay once.
TEST_F(PotionOfSpeedTest, TwoSpeedsHalveTheDelayOnce)
{
	player().activeBuffs.push_back(speed_ending_at(TIME_OF_DRINKING + ORDINARY_DELAY * 10));
	player().activeBuffs.push_back(speed_ending_at(TIME_OF_DRINKING + ORDINARY_DELAY * 20));

	EXPECT_EQ(player().get_action_delay(), HASTED_DELAY);
}

// "a character normally able to attack once per round attacks twice".
TEST_F(PotionOfSpeedTest, AHastedCreatureIsDueTwoActionsInARound)
{
	player().set_next_action_time(0);
	ctx.buffSystem->add_buff(player(), BuffType::SPEED, 0, 10, BuffApplication::ADD, 0);

	EXPECT_EQ(player().scheduled_actions_before(TIME_UNITS_PER_ROUND), 2);
}

// An ordinary action, a step or a swing, costs a hasted player half a round of the clock.
TEST_F(PotionOfSpeedTest, AHastedPlayersActionCostsHalfARound)
{
	ctx.buffSystem->add_buff(player(), BuffType::SPEED, 0, 10, BuffApplication::ADD, TIME_OF_DRINKING);

	spend_player_action(ctx, TIME_UNITS_PER_ROUND);

	EXPECT_EQ(ctx.gameState->get_time(), TIME_OF_DRINKING + HASTED_DELAY);
}

TEST_F(PotionOfSpeedTest, ThePotionIsASpeedBuffOf5d4Rounds)
{
	auto potion = ItemCreator::create("potion_of_speed", player().position, ctx);
	ASSERT_TRUE(std::holds_alternative<Consumable>(*potion->behavior));
	const auto& consumable = std::get<Consumable>(*potion->behavior);

	EXPECT_EQ(consumable.effect, ConsumableEffect::ADD_BUFF);
	EXPECT_EQ(consumable.buffType, BuffType::SPEED);
	EXPECT_EQ(to_text(consumable.duration), "5d4");
	EXPECT_EQ(consumable.durationUnit, DurationUnit::ROUNDS);
	EXPECT_EQ(consumable.agesYears, 1);
}

// The least of 5d4 is 5 rounds of game time, counted on the clock and not halved by the haste.
TEST_F(PotionOfSpeedTest, AtTheLeastThePotionLastsFiveRounds)
{
	drink_with_every_die_at(LOWEST_D4);

	EXPECT_EQ(player().get_action_delay(), HASTED_DELAY) << "drinking did not haste the drinker";
	EXPECT_TRUE(hasted_after(4));
	EXPECT_FALSE(hasted_after(5));
}

TEST_F(PotionOfSpeedTest, AtTheMostThePotionLastsTwentyRounds)
{
	drink_with_every_die_at(HIGHEST_D4);

	EXPECT_TRUE(hasted_after(19));
	EXPECT_FALSE(hasted_after(20));
}

// The save holds the base delay, and the halving comes back with the reloaded buff.
TEST_F(PotionOfSpeedTest, ASavedHastedCreatureKeepsItsBaseDelay)
{
	auto original = std::make_unique<Creature>(Vector2D{ 0, 0 }, ActorData{ TileRef{}, "goblin", ColorPairId::WHITE_BLACK });
	original->healthPool = std::make_unique<HealthPool>(10);
	original->armorClass = std::make_unique<ArmorClass>(10);
	ctx.buffSystem->add_buff(*original, BuffType::SPEED, 0, 10, BuffApplication::ADD, TIME_OF_DRINKING);
	nlohmann::json saved;
	original->save(saved);

	EXPECT_EQ(saved.at("actionDelay"), ORDINARY_DELAY) << "the hasted delay was saved as the base";

	auto loaded = std::make_unique<Creature>(Vector2D{ 0, 0 }, ActorData{ TileRef{}, "temp", ColorPairId::WHITE_BLACK });
	loaded->load(saved);
	EXPECT_EQ(loaded->get_unhasted_action_delay(), ORDINARY_DELAY);
	EXPECT_EQ(loaded->get_action_delay(), HASTED_DELAY) << "the reloaded buff did not haste the creature";
}

// "Use of a speed potion ages the individual by one year."
TEST_F(PotionOfSpeedTest, DrinkingAgesTheDrinkerByAYear)
{
	player().lifespan = Lifespan{ PlayerRaceState::HUMAN, 20, 100, 0, 0 };

	drink_with_every_die_at(LOWEST_D4);

	EXPECT_EQ(player().lifespan->magicalYears, 1);
	EXPECT_EQ(player().lifespan->physical_age(), 21);
	EXPECT_EQ(player().lifespan->natural_age(), 20) << "the potion's year was counted as time";
}

namespace
{

// "This does not reduce spellcasting time": a hasted player casting from the ring pays a full round.
[[noreturn]] void a_hasted_cast_costs_a_full_round()
{
	HiddenRenderer hidden{};
	MockGameContext mock{};
	GameContext ctx = mock.to_game_context();
	ctx.renderer = &hidden.renderer;
	make_player(mock, ctx);
	Player& player = *mock.player;
	auto ring = ItemCreator::create("ring_of_invisibility", player.position, ctx);
	[[maybe_unused]] const bool equipped = player.equip_item(std::move(ring), EquipmentSlot::LEFT_RING, ctx);
	ctx.gameState->set_time(TIME_OF_DRINKING);
	ctx.buffSystem->add_buff(player, BuffType::SPEED, 0, 10, BuffApplication::ADD, TIME_OF_DRINKING);
	exit_unless(player.get_action_delay() == HASTED_DELAY, "the player was not hasted before casting");

	mock.menus.push_back(std::make_unique<MenuSpellCast>(player, ctx));
	ctx.inputSystem->queue_key(GameKey::NONE, 'a');
	hidden.renderer.begin_frame();
	ctx.menuManager->handle_menus(*ctx.menus, ctx);
	hidden.renderer.end_frame();

	exit_unless(player.has_state(ActorState::IS_INVISIBLE), "the ring's invisibility was not cast");
	exit_unless(ctx.gameState->get_time() == TIME_OF_DRINKING + ORDINARY_DELAY, "a hasted casting did not cost a full round");
	std::_Exit(HIDDEN_RENDERER_PASSED);
}

} // namespace

TEST(PotionOfSpeedMenuTest, AHastedCastingCostsAFullRound)
{
	const int childExitCode = exit_code_in_child(&a_hasted_cast_costs_a_full_round);

	if (childExitCode == HIDDEN_RENDERER_NO_OPENGL_DRIVER)
	{
		GTEST_SKIP() << NO_OPENGL_DRIVER_REASON;
	}
	EXPECT_EQ(childExitCode, HIDDEN_RENDERER_PASSED) << "the child's check that failed is in its output above";
}

// end of file: PotionOfSpeedTest.cpp
