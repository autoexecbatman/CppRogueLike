// file: HelmCommandWordTest.cpp
// The helm of brilliance works only once its command word is spoken, and the word is
// learned by identifying the helm.
//
// What it is for. The DMG (PDF page 968): "When worn, it functions only upon the utterance
// of a special command word." The owner, 2026-10-21: the word "Must be discovered", and
// taking the helm off "Goes dormant". So a worn helm gives no armour and no fire
// protection until an identified one is spoken to from the cast menu, and it is dormant
// again whenever it comes off.
//
// The cast-menu cases run in a child process with tests/mocks/HiddenRenderer.h, and skip
// with the reason where there is no OpenGL driver.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=HelmCommandWordTest.*

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
#include "src/DamageInfo.h"
#include "src/DamageResolver.h"
#include "src/EquipmentSlot.h"
#include "src/ExperienceReward.h"
#include "src/GameContext.h"
#include "src/HealthPool.h"
#include "src/InputSystem.h"
#include "src/InventoryOperations.h"
#include "src/Item.h"
#include "src/ItemCreator.h"
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

// The armour class of a player wearing nothing.
constexpr int BARE_ARMOR_CLASS = 10;
// The helm is "armor of +2 value", which lowers armour class by two.
constexpr int EMPOWERED_ARMOR_CLASS = 8;
// "A double-strength fire resistance ring" is two rings' worth.
constexpr int EMPOWERED_FIRE_STRENGTH = 2;
// A creature built in a test has no Strength, and the pack is weight-gated by it.
constexpr int CARRIER_STRENGTH = 18;

// The player, wired into a context, with nothing worn and nothing in the pack.
void dress_bare_player(MockGameContext& mock, GameContext& ctx)
{
	mock.player = std::make_unique<Player>(Vector2D{ 0, 0 });
	Player& player = *mock.player;
	player.experienceReward = std::make_unique<ExperienceReward>(0);
	player.armorClass = std::make_unique<ArmorClass>(BARE_ARMOR_CLASS);
	player.healthPool = std::make_unique<HealthPool>(20);
	player.set_strength(CARRIER_STRENGTH);
	ctx.playerOwner = &mock.player;
}

// The helm of brilliance from the item data, unidentified, in the player's pack.
Item& carry_helm(Player& player, GameContext& ctx)
{
	auto helm = ItemCreator::create("helm_of_brilliance", player.position, ctx);
	Item* carried = helm.get();
	[[maybe_unused]] const auto added = InventoryOperations::add_item(player.inventoryData, std::move(helm));
	return *carried;
}

// The same helm with its type identified, which is how its word is learned.
Item& carry_identified_helm(Player& player, GameContext& ctx)
{
	Item& helm = carry_helm(player, ctx);
	helm.identify_type();
	return helm;
}

// Puts on or takes off an item the way the inventory's use does.
void use(Item& item, Player& player, GameContext& ctx)
{
	[[maybe_unused]] const bool used = use_item(*item.behavior, item, player, ctx);
}

HelmCommand command_of(const Item& helm)
{
	return std::get<MagicalHelm>(*helm.behavior).command;
}

bool offers_the_word(const Player& player)
{
	for (const SpellSystem::ItemGrantedSpell& granted : SpellSystem::get_item_granted_spells(player))
	{
		if (granted.power == SpellSystem::ItemPower::SPEAK_COMMAND_WORD)
		{
			return true;
		}
	}
	return false;
}

} // namespace

class HelmCommandWordTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();
		ctx.buffSystem = &buffs;
		dress_bare_player(mock, ctx);
	}

	Player& player() { return *mock.player; }

	int fire_strength() { return DamageResolver::resistance_strength(DamageType::FIRE, player(), ctx); }

	MockGameContext mock{};
	GameContext ctx{};
	BuffSystem buffs{};
};

TEST_F(HelmCommandWordTest, AnUnidentifiedHelmWornGivesNothingAndOffersNoWord)
{
	Item& helm = carry_helm(player(), ctx);

	use(helm, player(), ctx);
	ASSERT_EQ(player().get_equipped_item(EquipmentSlot::HEAD), &helm) << "the helm did not go on";

	EXPECT_EQ(player().get_armor_class(), BARE_ARMOR_CLASS) << "the helm's armour counts before its word is spoken";
	EXPECT_EQ(fire_strength(), 0) << "the helm's fire protection counts before its word is spoken";
	EXPECT_FALSE(offers_the_word(player())) << "an unidentified helm offered a word nobody knows";
}

TEST_F(HelmCommandWordTest, AnIdentifiedDormantHelmOffersItsWord)
{
	Item& helm = carry_identified_helm(player(), ctx);

	use(helm, player(), ctx);

	EXPECT_EQ(player().get_armor_class(), BARE_ARMOR_CLASS) << "the helm's armour counts before its word is spoken";
	EXPECT_EQ(fire_strength(), 0) << "the helm's fire protection counts before its word is spoken";
	EXPECT_TRUE(is_awaiting_command_word(helm));
	EXPECT_TRUE(offers_the_word(player())) << "an identified helm on the head offered no word to speak";
}

TEST_F(HelmCommandWordTest, SpeakingTheWordEmpowersTheHelm)
{
	Item& helm = carry_identified_helm(player(), ctx);
	use(helm, player(), ctx);

	speak_command_word(helm, player(), ctx);

	EXPECT_EQ(command_of(helm), HelmCommand::EMPOWERED);
	EXPECT_EQ(player().get_armor_class(), EMPOWERED_ARMOR_CLASS) << "the spoken helm's armour does not count";
	EXPECT_EQ(fire_strength(), EMPOWERED_FIRE_STRENGTH) << "the spoken helm's fire protection does not count";
	EXPECT_FALSE(offers_the_word(player())) << "a helm already spoken to offered its word again";
}

TEST_F(HelmCommandWordTest, TakingTheHelmOffSendsItDormant)
{
	Item& helm = carry_identified_helm(player(), ctx);
	use(helm, player(), ctx);
	speak_command_word(helm, player(), ctx);

	use(helm, player(), ctx);
	ASSERT_EQ(player().get_equipped_item(EquipmentSlot::HEAD), nullptr) << "the helm did not come off";
	use(helm, player(), ctx);

	EXPECT_EQ(command_of(helm), HelmCommand::DORMANT) << "the helm kept its empowerment through being taken off";
	EXPECT_EQ(player().get_armor_class(), BARE_ARMOR_CLASS);
	EXPECT_TRUE(offers_the_word(player())) << "the helm put back on did not ask for its word again";
}

TEST_F(HelmCommandWordTest, AHelmUnequippedFromTheEquipmentScreenGoesDormant)
{
	// The equipment screen takes a worn item off through unequip_item rather than use.
	Item& helm = carry_identified_helm(player(), ctx);
	use(helm, player(), ctx);
	std::get<MagicalHelm>(*helm.behavior).command = HelmCommand::EMPOWERED;

	ASSERT_TRUE(player().unequip_item(EquipmentSlot::HEAD, ctx));

	EXPECT_EQ(command_of(helm), HelmCommand::DORMANT) << "a helm taken off on the equipment screen kept its empowerment";
}

TEST_F(HelmCommandWordTest, AHelmTakenOffOnTheEquipmentScreenStopsProtectingAtOnce)
{
	Item& helm = carry_identified_helm(player(), ctx);
	use(helm, player(), ctx);
	speak_command_word(helm, player(), ctx);
	ASSERT_EQ(player().get_armor_class(), EMPOWERED_ARMOR_CLASS) << "the spoken helm never protected, so its removal shows nothing";

	ASSERT_TRUE(player().unequip_item(EquipmentSlot::HEAD, ctx));

	EXPECT_EQ(player().get_armor_class(), BARE_ARMOR_CLASS) << "the armour class still counts a helm that has come off";
}

TEST_F(HelmCommandWordTest, AnIdentifiedHelmInThePackOffersNoWord)
{
	[[maybe_unused]] Item& helm = carry_identified_helm(player(), ctx);

	EXPECT_FALSE(offers_the_word(player())) << "a helm nobody is wearing offered its word";
}

TEST_F(HelmCommandWordTest, AHelmOfTeleportationStillGrantsTeleport)
{
	// Identified, so the only thing keeping a command word off it is that it has none.
	auto helm = ItemCreator::create("helm_of_teleportation", player().position, ctx);
	helm->identify_type();
	ASSERT_TRUE(player().equip_item(std::move(helm), EquipmentSlot::HEAD, ctx));

	const std::vector<SpellSystem::ItemGrantedSpell> granted = SpellSystem::get_item_granted_spells(player());

	ASSERT_EQ(granted.size(), 1u);
	EXPECT_EQ(granted.front().key, "teleport");
	EXPECT_EQ(granted.front().power, SpellSystem::ItemPower::CAST_SPELL);
}

TEST_F(HelmCommandWordTest, AnEmpoweredHelmSurvivesASave)
{
	const ItemBehavior empowered = MagicalHelm{ MagicalEffect::BRILLIANCE, 2, HelmCommand::EMPOWERED };
	json record;
	save_behavior(empowered, record);

	const ItemBehavior loaded = load_behavior(record);

	EXPECT_EQ(std::get<MagicalHelm>(loaded).command, HelmCommand::EMPOWERED) << "the save lost the spoken word";
}

TEST_F(HelmCommandWordTest, AHelmRecordWithoutItsCommandIsRefused)
{
	json record;
	save_behavior(MagicalHelm{ MagicalEffect::BRILLIANCE, 2, HelmCommand::DORMANT }, record);
	record.erase("command");

	EXPECT_THROW((void)load_behavior(record), std::exception) << "a helm record missing its command loaded with a default";
}

namespace
{

// Through the cast menu, as a player does it: Shift+C, then the word's letter. The word is
// spoken and a turn is spent.
[[noreturn]] void speaking_from_the_cast_menu_spends_a_turn()
{
	HiddenRenderer hidden{};
	MockGameContext mock{};
	GameContext ctx = mock.to_game_context();
	ctx.renderer = &hidden.renderer;
	BuffSystem buffs{};
	ctx.buffSystem = &buffs;
	dress_bare_player(mock, ctx);
	Item& helm = carry_identified_helm(*mock.player, ctx);
	use(helm, *mock.player, ctx);
	const int timeBefore = ctx.gameState->get_time();

	mock.menus.push_back(std::make_unique<MenuSpellCast>(*mock.player, ctx));
	ctx.inputSystem->queue_key(GameKey::NONE, 'a');
	hidden.renderer.begin_frame();
	ctx.menuManager->handle_menus(*ctx.menus, ctx);
	hidden.renderer.end_frame();

	exit_unless(command_of(helm) == HelmCommand::EMPOWERED, "choosing the word from the cast menu did not speak it");
	exit_unless(ctx.gameState->get_time() > timeBefore, "speaking the word spent no turn");
	std::_Exit(HIDDEN_RENDERER_PASSED);
}

} // namespace

TEST(HelmCommandWordMenuTest, SpeakingFromTheCastMenuSpendsATurn)
{
	const int childExitCode = exit_code_in_child(&speaking_from_the_cast_menu_spends_a_turn);

	if (childExitCode == HIDDEN_RENDERER_NO_OPENGL_DRIVER)
	{
		GTEST_SKIP() << NO_OPENGL_DRIVER_REASON;
	}
	EXPECT_EQ(childExitCode, HIDDEN_RENDERER_PASSED) << "the child's check that failed is in its output above";
}
