// file: HelmRechargeTest.cpp
// The helm of teleportation teleports its wearer once a day; the ring of invisibility
// casts at will.
//
// What it is for. The DMG gives the helm "once per day" (PDF page 970) and the ring "at
// will" (PDF page 908). The driver's reading, 2026-10-21: a day is twenty-four hours of
// game time from the last use. So a helm used once is offered again no sooner than a day
// later, and the ring is offered however often it has been used.
//
// The cast-menu cases run in a child process with tests/mocks/HiddenRenderer.h, and skip
// with the reason where there is no OpenGL driver.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=HelmRecharge*

#include <gtest/gtest.h>

#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <variant>
#include <vector>

#include <nlohmann/json.hpp>

#include "src/ArmorClass.h"
#include "src/BuffSystem.h"
#include "src/Controls.h"
#include "src/EquipmentSlot.h"
#include "src/ExperienceReward.h"
#include "src/GameContext.h"
#include "src/HealthPool.h"
#include "src/InputSystem.h"
#include "src/Item.h"
#include "src/ItemCreator.h"
#include "src/Map.h"
#include "src/MenuManager.h"
#include "src/MenuSpellCast.h"
#include "src/Pickable.h"
#include "src/Player.h"
#include "src/Renderer.h"
#include "src/SpellSystem.h"
#include "tests/mocks/HiddenRenderer.h"
#include "tests/mocks/MockGameContext.h"

namespace
{

// A day in the clock's units, from the book rather than from the code's constants: 24
// hours of 60 one-minute rounds (PHB page 184), each round 120 time units.
constexpr int DAY_IN_TIME_UNITS = 172800;
// A moment well into a game, so a use at time zero cannot pass for no use at all.
constexpr int TIME_OF_USE = 5000;
// One tile of floor away from the player, so a teleport has somewhere to land.
constexpr Vector2D LANDING_TILE{ 3, 3 };

// The player, wired into a context, with nothing worn.
void dress_bare_player(MockGameContext& mock, GameContext& ctx)
{
	mock.player = std::make_unique<Player>(Vector2D{ 0, 0 });
	Player& player = *mock.player;
	player.experienceReward = std::make_unique<ExperienceReward>(0);
	player.armorClass = std::make_unique<ArmorClass>(10);
	player.healthPool = std::make_unique<HealthPool>(20);
	player.set_strength(10);
	player.set_dexterity(10);
	ctx.playerOwner = &mock.player;
}

// Puts an item from the item data on the player, and returns it as worn.
Item& wear(
	std::string_view itemKey,
	EquipmentSlot slot,
	Player& player,
	GameContext& ctx)
{
	auto item =
		ItemCreator::create(std::string{ itemKey }, player.position, ctx);
	Item* worn = item.get();
	[[maybe_unused]] const bool equipped =
		player.equip_item(std::move(item), slot, ctx);
	return *worn;
}

// Whether the cast menu would offer this spell at this moment.
bool offers(std::string_view spellKey, const Player& player, int now)
{
	for (
		const SpellSystem::ItemGrantedSpell& granted :
		SpellSystem::get_item_granted_spells(player, now))
	{
		if (granted.key == spellKey)
		{
			return true;
		}
	}
	return false;
}

} // namespace

class HelmRechargeTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();
		ctx.buffSystem = &buffs;
		dress_bare_player(mock, ctx);
	}

	Player& player() { return *mock.player; }

	MockGameContext mock{};
	GameContext ctx{};
	BuffSystem buffs{};
};

TEST_F(HelmRechargeTest, AnUnusedHelmOffersItsTeleport)
{
	Item& helm =
		wear("helm_of_teleportation", EquipmentSlot::HEAD, player(), ctx);

	EXPECT_TRUE(is_helm_ready(helm, TIME_OF_USE));
	EXPECT_TRUE(offers("teleport", player(), TIME_OF_USE))
		<< "a helm never used did not offer its teleport";
}

TEST_F(HelmRechargeTest, AUsedHelmOffersNothingForADay)
{
	Item& helm =
		wear("helm_of_teleportation", EquipmentSlot::HEAD, player(), ctx);

	record_helm_use(helm, TIME_OF_USE);

	EXPECT_FALSE(is_helm_ready(helm, TIME_OF_USE))
		<< "the helm was ready again the moment it was used";
	EXPECT_FALSE(offers("teleport", player(), TIME_OF_USE))
		<< "the menu offered a teleport the helm had just spent";
	EXPECT_FALSE(
		offers("teleport", player(), TIME_OF_USE + DAY_IN_TIME_UNITS - 1))
		<< "the helm recharged before a day had passed";
}

TEST_F(HelmRechargeTest, AUsedHelmIsOfferedAgainADayLater)
{
	Item& helm =
		wear("helm_of_teleportation", EquipmentSlot::HEAD, player(), ctx);

	record_helm_use(helm, TIME_OF_USE);

	EXPECT_TRUE(is_helm_ready(helm, TIME_OF_USE + DAY_IN_TIME_UNITS));
	EXPECT_TRUE(offers("teleport", player(), TIME_OF_USE + DAY_IN_TIME_UNITS))
		<< "the helm had not recharged a day after its use";
}

TEST_F(HelmRechargeTest, AHelmWithoutTheDailyPowerIsNeverReady)
{
	Item& helm = wear("helm_of_brilliance", EquipmentSlot::HEAD, player(), ctx);

	EXPECT_FALSE(is_helm_ready(helm, TIME_OF_USE))
		<< "a helm of brilliance answered as though it had a daily teleport";
}

TEST_F(HelmRechargeTest, TheHelmsTeleportIsOfferedAsADailyPower)
{
	[[maybe_unused]] Item& helm =
		wear("helm_of_teleportation", EquipmentSlot::HEAD, player(), ctx);

	const std::vector<SpellSystem::ItemGrantedSpell> granted =
		SpellSystem::get_item_granted_spells(player(), TIME_OF_USE);

	ASSERT_EQ(granted.size(), 1u);
	EXPECT_EQ(
		granted.front().power,
		SpellSystem::ItemPower::CAST_SPELL_ONCE_A_DAY);
}

TEST_F(HelmRechargeTest, AUsedHelmSurvivesASave)
{
	Item& helm =
		wear("helm_of_teleportation", EquipmentSlot::HEAD, player(), ctx);
	record_helm_use(helm, TIME_OF_USE);
	json record;
	save_behavior(*helm.behavior, record);

	const ItemBehavior loaded = load_behavior(record);

	EXPECT_EQ(
		std::get<MagicalHelm>(loaded).rechargedAt,
		TIME_OF_USE + DAY_IN_TIME_UNITS)
		<< "the save lost when the helm recharges";
}

TEST_F(HelmRechargeTest, AHelmRecordWithoutItsRechargeIsRefused)
{
	json record;
	save_behavior(
		MagicalHelm{ MagicalEffect::TELEPORTATION, 0, HelmCommand::DORMANT },
		record);
	record.erase("rechargedAt");

	EXPECT_THROW((void)load_behavior(record), std::exception)
		<< "a helm record missing its recharge time loaded with a default";
}

namespace
{

// Shift+C, then the first entry's letter, in one frame.
void choose_first_entry_from_the_cast_menu(
	MockGameContext& mock,
	GameContext& ctx,
	HiddenRenderer& hidden)
{
	mock.menus.push_back(std::make_unique<MenuSpellCast>(*mock.player, ctx));
	ctx.inputSystem->queue_key(GameKey::NONE, 'a');
	hidden.renderer.begin_frame();
	ctx.menuManager->handle_menus(*ctx.menus, ctx);
	hidden.renderer.end_frame();
}

// Teleporting through the helm from the cast menu spends its day.
[[noreturn]] void teleporting_from_the_cast_menu_spends_the_helm()
{
	HiddenRenderer hidden{};
	MockGameContext mock{};
	GameContext ctx = mock.to_game_context();
	ctx.renderer = &hidden.renderer;
	BuffSystem buffs{};
	ctx.buffSystem = &buffs;
	mock.map.init_tiles();
	mock.map.set_tile(LANDING_TILE, TileType::FLOOR);
	dress_bare_player(mock, ctx);
	Item& helm =
		wear("helm_of_teleportation", EquipmentSlot::HEAD, *mock.player, ctx);
	ctx.gameState->set_time(TIME_OF_USE);

	choose_first_entry_from_the_cast_menu(mock, ctx, hidden);

	exit_unless(
		ctx.gameState->get_time() > TIME_OF_USE,
		"teleporting from the helm spent no turn");
	exit_unless(
		std::get<MagicalHelm>(*helm.behavior).rechargedAt ==
			TIME_OF_USE + DAY_IN_TIME_UNITS,
		"the helm's use was not recorded at the moment of use");
	exit_unless(
		!offers("teleport", *mock.player, ctx.gameState->get_time()),
		"the menu offers the helm's teleport again straight after its use");
	finish_the_child();
}

// The menu lists as of the clock: a day after a use, it offers the helm and casting spends
// the new day.
[[noreturn]] void the_menu_offers_the_helm_again_a_day_after_its_use()
{
	HiddenRenderer hidden{};
	MockGameContext mock{};
	GameContext ctx = mock.to_game_context();
	ctx.renderer = &hidden.renderer;
	BuffSystem buffs{};
	ctx.buffSystem = &buffs;
	mock.map.init_tiles();
	mock.map.set_tile(LANDING_TILE, TileType::FLOOR);
	dress_bare_player(mock, ctx);
	Item& helm =
		wear("helm_of_teleportation", EquipmentSlot::HEAD, *mock.player, ctx);
	record_helm_use(helm, TIME_OF_USE);
	const int aDayLater = TIME_OF_USE + DAY_IN_TIME_UNITS;
	ctx.gameState->set_time(aDayLater);

	choose_first_entry_from_the_cast_menu(mock, ctx, hidden);

	exit_unless(
		std::get<MagicalHelm>(*helm.behavior).rechargedAt ==
			aDayLater + DAY_IN_TIME_UNITS,
		"the menu a day later did not cast the helm's teleport");
	finish_the_child();
}

// The ring is at will: cast from the menu, it is offered again at once.
[[noreturn]] void the_ring_is_offered_again_after_its_cast()
{
	HiddenRenderer hidden{};
	MockGameContext mock{};
	GameContext ctx = mock.to_game_context();
	ctx.renderer = &hidden.renderer;
	BuffSystem buffs{};
	ctx.buffSystem = &buffs;
	dress_bare_player(mock, ctx);
	[[maybe_unused]] Item& ring = wear(
		"ring_of_invisibility",
		EquipmentSlot::LEFT_RING,
		*mock.player,
		ctx);
	ctx.gameState->set_time(TIME_OF_USE);

	choose_first_entry_from_the_cast_menu(mock, ctx, hidden);

	exit_unless(
		mock.player->has_state(ActorState::IS_INVISIBLE),
		"casting from the ring did not make its wearer invisible");
	exit_unless(
		offers("invisibility", *mock.player, ctx.gameState->get_time()),
		"the ring was not offered again after its cast");
	finish_the_child();
}

} // namespace

TEST(HelmRechargeMenuTest, TeleportingFromTheCastMenuSpendsTheHelm)
{
	const int childExitCode =
		exit_code_in_child(&teleporting_from_the_cast_menu_spends_the_helm);

	if (childExitCode == HIDDEN_RENDERER_NO_OPENGL_DRIVER)
	{
		GTEST_SKIP() << NO_OPENGL_DRIVER_REASON;
	}
	EXPECT_EQ(childExitCode, HIDDEN_RENDERER_PASSED)
		<< "the child's check that failed is in its output above";
}

TEST(HelmRechargeMenuTest, TheMenuOffersTheHelmAgainADayAfterItsUse)
{
	const int childExitCode =
		exit_code_in_child(&the_menu_offers_the_helm_again_a_day_after_its_use);

	if (childExitCode == HIDDEN_RENDERER_NO_OPENGL_DRIVER)
	{
		GTEST_SKIP() << NO_OPENGL_DRIVER_REASON;
	}
	EXPECT_EQ(childExitCode, HIDDEN_RENDERER_PASSED)
		<< "the child's check that failed is in its output above";
}

TEST(HelmRechargeMenuTest, TheRingIsOfferedAgainAfterItsCast)
{
	const int childExitCode =
		exit_code_in_child(&the_ring_is_offered_again_after_its_cast);

	if (childExitCode == HIDDEN_RENDERER_NO_OPENGL_DRIVER)
	{
		GTEST_SKIP() << NO_OPENGL_DRIVER_REASON;
	}
	EXPECT_EQ(childExitCode, HIDDEN_RENDERER_PASSED)
		<< "the child's check that failed is in its output above";
}
