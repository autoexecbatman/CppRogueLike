// file: BootsAndCloaksTest.cpp
// Boots and cloaks are their own kinds of item: they are listed under their own
// headings, worn only in their own slots, and saved and loaded as what they
// are.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=BootsAndCloaksTest.*

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <string_view>

#include "src/ArmorClass.h"
#include "src/EquipmentSlot.h"
#include "src/ExperienceReward.h"
#include "src/Game.h"
#include "src/HealthPool.h"
#include "src/InventoryUI.h"
#include "src/Item.h"
#include "src/ItemClassification.h"
#include "src/ItemCreator.h"
#include "src/ItemRegistry.h"
#include "src/Paths.h"
#include "src/Player.h"
#include "tests/mocks/TestDice.h"

using json = nlohmann::json;

namespace
{
// The records items.json ships for each kind.
constexpr std::array<std::string_view, 2> BOOT_KEYS{
	"boots_of_elvenkind",
	"boots_of_speed",
};
constexpr std::array<std::string_view, 3> CLOAK_KEYS{
	"cloak_of_displacement",
	"cloak_of_elvenkind",
	"cloak_of_protection",
};
} // namespace

class BootsAndCloaksTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		game.dataManager.load_all_data(game.messageSystem);
		game.tileConfig.load(Paths::TILE_CONFIG);
		game.itemRegistry.load(Paths::ITEMS);
		player->experienceReward = std::make_unique<ExperienceReward>(0);
		player->healthPool = std::make_unique<HealthPool>(20);
		player->armorClass = std::make_unique<ArmorClass>(10);
		ctx = game.context();
		ctx.playerOwner = &player;
	}

	std::unique_ptr<Item> make(std::string_view key)
	{
		return ItemCreator::create(key, Vector2D{ 0, 0 }, ctx);
	}

	Game game{ TEST_DICE_SEED };
	GameContext ctx;
	std::unique_ptr<Player> player{ std::make_unique<Player>(
		Vector2D{ 0, 0 }) };
};

TEST_F(BootsAndCloaksTest, EveryBootRecordIsABootAndEveryCloakRecordACloak)
{
	for (const std::string_view key : BOOT_KEYS)
	{
		const ItemParams& params = ctx.itemRegistry->get_params(key);
		EXPECT_EQ(params.itemClass, ItemClass::BOOTS) << key;
		EXPECT_EQ(
			ItemClassificationUtils::get_category(params.itemClass),
			ItemCategory::BOOTS)
			<< key;
	}
	for (const std::string_view key : CLOAK_KEYS)
	{
		const ItemParams& params = ctx.itemRegistry->get_params(key);
		EXPECT_EQ(params.itemClass, ItemClass::CLOAK) << key;
		EXPECT_EQ(
			ItemClassificationUtils::get_category(params.itemClass),
			ItemCategory::CLOAK)
			<< key;
	}
}

TEST_F(BootsAndCloaksTest, TheBackpackHeadsBootsAndCloaksByTheirOwnNames)
{
	EXPECT_EQ(InventoryActions::category_name(ItemCategory::BOOTS), "Boots");
	EXPECT_EQ(InventoryActions::category_name(ItemCategory::CLOAK), "Cloaks");
	EXPECT_EQ(
		InventoryActions::category_name(ItemCategory::GAUNTLETS),
		"Gauntlets");
}

// The backpack lists only the categories in its order, so one left out hides
// every item filed under it.
TEST_F(BootsAndCloaksTest, EveryClassesCategoryHasOnePlaceInTheBackpack)
{
	for (const ItemClassRecord& record : ITEM_CLASS_TABLE)
	{
		EXPECT_EQ(std::ranges::count(CATEGORY_ORDER, record.category), 1)
			<< record.key;
	}
}

TEST_F(BootsAndCloaksTest, TheGauntletsSlotTakesGauntletsAndNotBoots)
{
	const auto boots = make("boots_of_speed");
	const auto gauntlets = make("gauntlets_of_dexterity");

	EXPECT_FALSE(player->can_equip(*boots, EquipmentSlot::GAUNTLETS));
	EXPECT_TRUE(player->can_equip(*gauntlets, EquipmentSlot::GAUNTLETS));
}

TEST_F(BootsAndCloaksTest, TheBootsSlotTakesBootsAndNotGauntlets)
{
	const auto boots = make("boots_of_speed");
	const auto gauntlets = make("gauntlets_of_dexterity");

	EXPECT_TRUE(player->can_equip(*boots, EquipmentSlot::BOOTS));
	EXPECT_FALSE(player->can_equip(*gauntlets, EquipmentSlot::BOOTS));
}

TEST_F(BootsAndCloaksTest, TheCloakSlotTakesACloakAndNotBoots)
{
	const auto cloak = make("cloak_of_protection");
	const auto boots = make("boots_of_speed");

	EXPECT_TRUE(player->can_equip(*cloak, EquipmentSlot::CLOAK));
	EXPECT_FALSE(player->can_equip(*boots, EquipmentSlot::CLOAK));
}

// A save names the class by its key, so a class added in the middle of the
// enum cannot turn a saved item into its neighbour.
TEST_F(BootsAndCloaksTest, ASavedBootIsWrittenByNameAndLoadsAsABoot)
{
	const auto boots = make("boots_of_speed");
	json saved;
	boots->save(saved);

	Item loaded{
		Vector2D{ 0, 0 },
		ActorData{ TileRef{}, "temp", ColorPairId::WHITE_BLACK },
	};
	loaded.load(saved);

	EXPECT_EQ(saved.at("itemClass"), "boots");
	EXPECT_EQ(loaded.itemClass, ItemClass::BOOTS);
}

TEST_F(BootsAndCloaksTest, EveryClassEncodesToAKeyThatParsesBackToIt)
{
	for (const ItemClass itemClass : ALL_ITEM_CLASS)
	{
		EXPECT_EQ(parse_item_class(encode_item_class(itemClass)), itemClass)
			<< encode_item_class(itemClass);
	}
	EXPECT_EQ(parse_item_class("boots"), ItemClass::BOOTS);
	EXPECT_EQ(parse_item_class("cloak"), ItemClass::CLOAK);
}

// end of file: BootsAndCloaksTest.cpp
