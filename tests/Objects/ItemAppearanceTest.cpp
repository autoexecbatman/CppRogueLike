// file: ItemAppearanceTest.cpp
// An item whose type is unidentified is called by its appearance, the name its
// record gives for how it looks, and by its true name once identified. The DMG
// says a helm of brilliance "appears to be nothing more than an ordinary piece
// of armor for head protection"; a mundane item's appearance is its own name.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=ItemAppearanceTest.*

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <cassert>
#include <filesystem>
#include <fstream>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>

#include "src/ArmorClass.h"
#include "src/CreatureClass.h"
#include "src/EquipmentSlot.h"
#include "src/ExperienceReward.h"
#include "src/Game.h"
#include "src/HealthPool.h"
#include "src/InventoryOperations.h"
#include "src/Item.h"
#include "src/ItemClassification.h"
#include "src/ItemCreator.h"
#include "src/ItemRegistry.h"
#include "src/Paths.h"
#include "src/Pickable.h"
#include "src/Player.h"
#include "tests/mocks/TestDice.h"

using json = nlohmann::json;

class ItemAppearanceTest : public ::testing::Test
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
		player->set_strength(16);
		player->set_dexterity(10);
		ctx = game.context();
		ctx.playerOwner = &player;
	}

	// Puts an item from the data on through the inventory screen's path.
	Item& wear(std::string_view key, EquipmentSlot slot)
	{
		auto item = ItemCreator::create(key, player->position, ctx);
		Item* carried = item.get();
		[[maybe_unused]] const auto added =
			InventoryOperations::add_item_to_inventory(
				player->inventoryData,
				std::move(item),
				*player,
				*ctx.dataManager);
		use_item(*carried->behavior, *carried, *player, ctx);
		EXPECT_EQ(player->get_equipped_item(slot), carried)
			<< key << " was not put on";
		return *carried;
	}

	Game game{ TEST_DICE_SEED };
	GameContext ctx;
	std::unique_ptr<Player> player{ std::make_unique<Player>(
		Vector2D{ 0, 0 }) };
};

TEST_F(ItemAppearanceTest, AnUnidentifiedHelmOfBrillianceIsCalledAHelm)
{
	auto helm =
		ItemCreator::create("helm_of_brilliance", Vector2D{ 0, 0 }, ctx);

	EXPECT_EQ(helm->get_name(), "helm");
	helm->identify_all();
	EXPECT_EQ(helm->get_name(), "helm of brilliance");
}

TEST_F(ItemAppearanceTest, AnUnidentifiedDaggerIsCalledADagger)
{
	auto dagger = ItemCreator::create("dagger", Vector2D{ 0, 0 }, ctx);

	EXPECT_EQ(dagger->get_name(), "dagger");
}

// With every potion called "potion" the player could not tell two kinds
// apart, so each potion and each scroll looks like no other of its class.
TEST_F(ItemAppearanceTest, EachPotionAndScrollTypeLooksLikeNoOther)
{
	std::set<std::string> potionLooks{};
	std::set<std::string> scrollLooks{};
	int potions = 0;
	int scrolls = 0;
	for (const std::string& key : ctx.itemRegistry->get_all_keys())
	{
		const ItemParams& params = ctx.itemRegistry->get_params(key);
		if (params.itemClass == ItemClass::POTION)
		{
			++potions;
			potionLooks.insert(params.appearance);
			EXPECT_NE(params.appearance, params.name)
				<< key << " looks like itself";
		}
		if (params.itemClass == ItemClass::SCROLL)
		{
			++scrolls;
			scrollLooks.insert(params.appearance);
			EXPECT_NE(params.appearance, params.name)
				<< key << " looks like itself";
		}
	}
	EXPECT_GT(potions, 1) << "the data holds too few potions to compare";
	EXPECT_EQ(potionLooks.size(), static_cast<size_t>(potions));
	EXPECT_EQ(scrollLooks.size(), static_cast<size_t>(scrolls));
}

// The armour class breakdown lists what each worn item gave, so it names a
// worn ring as the player knows it.
TEST_F(ItemAppearanceTest, AWornUnidentifiedRingIsListedByItsAppearance)
{
	wear("ring_of_protection_plus_1", EquipmentSlot::RIGHT_RING);

	const ArmorClassBreakdown ac = player->armorClass->update(*player, ctx);

	EXPECT_EQ(ac.ring.name, "ring");
}

TEST_F(ItemAppearanceTest, AnItemsAppearanceSurvivesASave)
{
	auto helm =
		ItemCreator::create("helm_of_brilliance", Vector2D{ 0, 0 }, ctx);
	json saved;
	helm->save(saved);

	Item loaded{
		Vector2D{ 0, 0 },
		ActorData{ TileRef{}, "temp", ColorPairId::WHITE_BLACK },
	};
	loaded.load(saved);

	EXPECT_EQ(loaded.get_name(), "helm");
}

TEST_F(ItemAppearanceTest, AnItemRecordWithNoAppearanceIsRefused)
{
	std::ifstream shipped(Paths::resolve(Paths::ITEMS));
	ASSERT_TRUE(shipped.is_open()) << "cannot read " << Paths::ITEMS;
	json document = json::parse(shipped);
	document.at("dagger").erase("appearance");
	const std::filesystem::path scratch =
		std::filesystem::temp_directory_path() /
		"ItemAppearanceTest_items.json";
	{
		std::ofstream out(scratch);
		out << document.dump(4);
	}

	ItemRegistry registry;
	std::string refusal{};
	try
	{
		registry.load(scratch.string());
	}
	catch (const std::runtime_error& error)
	{
		refusal = error.what();
	}
	std::filesystem::remove(scratch);

	EXPECT_NE(refusal.find("appearance"), std::string::npos)
		<< "the record with no appearance loaded: \"" << refusal << "\"";
}

// The item editor saves through the registry's own writer, so a field it
// dropped would make the next load refuse the whole file.
TEST_F(ItemAppearanceTest, TheRegistrysOwnSaveKeepsEveryAppearance)
{
	const std::filesystem::path scratch =
		std::filesystem::temp_directory_path() /
		"ItemAppearanceTest_saved_items.json";
	ctx.itemRegistry->save(scratch.string());

	ItemRegistry reloaded;
	reloaded.load(scratch.string());
	std::filesystem::remove(scratch);

	EXPECT_EQ(reloaded.get_params("helm_of_brilliance").appearance, "helm");
	for (const std::string& key : ctx.itemRegistry->get_all_keys())
	{
		EXPECT_EQ(
			reloaded.get_params(key).appearance,
			ctx.itemRegistry->get_params(key).appearance)
			<< key;
	}
}

// An item built from no record, as a corpse is, looks like what it is.
TEST_F(ItemAppearanceTest, AnItemBuiltWithoutARecordIsCalledByItsName)
{
	const Item corpse{
		Vector2D{ 0, 0 },
		ActorData{ TileRef{}, "kobold corpse", ColorPairId::WHITE_BLACK },
	};

	EXPECT_EQ(corpse.get_name(), "kobold corpse");
}

// The editor's new item looks like itself until its appearance is authored.
TEST_F(ItemAppearanceTest, ACustomItemLooksLikeItsName)
{
	const std::string key = ctx.itemRegistry->add_custom(
		"Registry Trinket",
		"potion",
		ItemParams{});

	EXPECT_EQ(ctx.itemRegistry->get_params(key).appearance, "Registry Trinket");
}

// The editor commits through set_params and saves through the registry, so an
// appearance authored that way is what the next load reads.
TEST_F(ItemAppearanceTest, AnAppearanceSetThroughTheRegistryIsSavedAndLoaded)
{
	ItemParams edited = ctx.itemRegistry->get_params("helm_of_brilliance");
	edited.appearance = "dented helm";
	ctx.itemRegistry->set_params("helm_of_brilliance", edited);
	const std::filesystem::path scratch =
		std::filesystem::temp_directory_path() /
		"ItemAppearanceTest_edited_items.json";
	ctx.itemRegistry->save(scratch.string());

	ItemRegistry reloaded;
	reloaded.load(scratch.string());
	std::filesystem::remove(scratch);

	EXPECT_EQ(
		reloaded.get_params("helm_of_brilliance").appearance,
		"dented helm");
}

// A character knows what they packed, so a starting kit is identified; a
// scroll of the same type found later is still known only by its look.
TEST_F(ItemAppearanceTest, AFightersOwnFireballScrollIsKnownAndAFoundOneIsNot)
{
	player->set_creature_class(CreatureClass::FIGHTER);
	player->equip_class_starting_gear(ctx);
	const Item* packed = nullptr;
	for (const auto& item : player->inventoryData.items)
	{
		assert(item && "the starting pack holds a null item");
		if (item->itemKey == "scroll_fireball")
		{
			packed = item.get();
		}
	}
	ASSERT_NE(packed, nullptr) << "a fighter starts with no fireball scroll";

	auto found = ItemCreator::create("scroll_fireball", Vector2D{ 0, 0 }, ctx);

	EXPECT_EQ(packed->get_name(), "scroll of fireball");
	EXPECT_EQ(
		found->get_name(),
		ctx.itemRegistry->get_params("scroll_fireball").appearance);
	EXPECT_NE(found->get_name(), "scroll of fireball");
}

// end of file: ItemAppearanceTest.cpp
