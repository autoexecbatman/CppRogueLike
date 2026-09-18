// Checks what wearing and removing ability-raising equipment does to an ability score.
//
// What it is for. Gauntlets, girdles and amulets that raise an ability either add to it -
// gauntlets of swimming and climbing, +2 Strength - or set it, as a girdle of giant
// strength does: "The Strength gained is not cumulative with normal or magical Strength
// bonuses" (Dungeon Master's Guide, PDF page 966 of the 2e archive). While worn the score
// is the higher of the creature's own and the highest set value, plus every addition, the
// rule buffs already follow; and taking an item off must return the score to exactly what
// it was, in whatever order items and buffs come and go.
//
// Items are put on the way the inventory screen does - use_item on the item - and taken
// off the way the equipment screen does, unequip_item on the slot.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=StatBoostEquipmentTest.*

#include <gtest/gtest.h>

#include <memory>
#include <string_view>

#include "src/ArmorClass.h"
#include "src/BuffSystem.h"
#include "src/BuffType.h"
#include "src/Creature.h"
#include "src/EquipmentSlot.h"
#include "src/ExperienceReward.h"
#include "src/Game.h"
#include "src/HealthPool.h"
#include "src/InventoryOperations.h"
#include "src/Item.h"
#include "src/ItemCreator.h"
#include "src/Paths.h"
#include "src/Pickable.h"
#include "src/Player.h"

class StatBoostEquipmentTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		game.dataManager.load_all_data(game.messageSystem);
		game.tileConfig.load(Paths::TILE_CONFIG);
		ItemCreator::load(Paths::ITEMS);

		player->experienceReward = std::make_unique<ExperienceReward>(0);
		player->healthPool = std::make_unique<HealthPool>(20);
		player->armorClass = std::make_unique<ArmorClass>(10);
		player->set_strength(BASE_STRENGTH);
		player->set_dexterity(10);
		player->set_constitution(10);

		ctx = game.context();
		ctx.playerOwner = &player;
	}

	// Puts an item from the data into the pack and uses it, as the inventory screen does.
	void put_on(std::string_view key)
	{
		auto item = ItemCreator::create(key, player->position, *ctx.contentRegistry);
		Item* carried = item.get();
		[[maybe_unused]] const auto added = InventoryOperations::add_item_to_inventory(player->inventoryData, std::move(item), *player);
		ASSERT_TRUE(added.has_value()) << key << " did not fit in the pack";
		use_item(*carried->behavior, *carried, *player, ctx);
		ASSERT_NE(player->get_equipped_item(slot_of(key)), nullptr) << key << " was not put on";
	}

	// Takes off whatever is in the slot, as the equipment screen does.
	void take_off(EquipmentSlot slot)
	{
		ASSERT_TRUE(player->unequip_item(slot, ctx));
	}

	static EquipmentSlot slot_of(std::string_view key)
	{
		if (key.starts_with("girdle"))
		{
			return EquipmentSlot::GIRDLE;
		}
		if (key.starts_with("amulet"))
		{
			return EquipmentSlot::NECK;
		}
		return EquipmentSlot::GAUNTLETS;
	}

	static constexpr int BASE_STRENGTH = 12;

	Game game;
	GameContext ctx;
	std::unique_ptr<Player> player{ std::make_unique<Player>(Vector2D{ 0, 0 }) };
};

// Gauntlets of swimming and climbing add +2, once.
TEST_F(StatBoostEquipmentTest, AnAddingItemAddsOnce)
{
	put_on("gauntlets_of_swimming_and_climbing");

	EXPECT_EQ(player->get_strength(), BASE_STRENGTH + 2);
}

// Taking them off returns exactly what they gave.
TEST_F(StatBoostEquipmentTest, RemovingAnAddingItemRestoresTheScore)
{
	put_on("gauntlets_of_swimming_and_climbing");
	take_off(EquipmentSlot::GAUNTLETS);

	EXPECT_EQ(player->get_strength(), BASE_STRENGTH);
}

// Gauntlets of ogre power set Strength to 18 while worn, and give it back when removed.
TEST_F(StatBoostEquipmentTest, RemovingASettingItemRestoresTheScore)
{
	put_on("gauntlets_of_ogre_power");
	ASSERT_EQ(player->get_strength(), 18);

	take_off(EquipmentSlot::GAUNTLETS);

	EXPECT_EQ(player->get_strength(), BASE_STRENGTH) << "Strength kept the gauntlets' 18 after they came off";
}

// Setting to 18 does nothing for a creature already at 19.
TEST_F(StatBoostEquipmentTest, ASettingItemNeverLowersAHigherScore)
{
	player->set_strength(19);

	put_on("gauntlets_of_ogre_power");

	EXPECT_EQ(player->get_strength(), 19);
}

// Two setting items: the higher wins while both are worn, and removing either leaves the other's.
TEST_F(StatBoostEquipmentTest, TwoSettingItemsComeOffInEitherOrder)
{
	put_on("gauntlets_of_ogre_power");
	put_on("girdle_of_hill_giant_strength");
	ASSERT_EQ(player->get_strength(), 19);

	take_off(EquipmentSlot::GAUNTLETS);
	EXPECT_EQ(player->get_strength(), 19) << "the girdle is still worn";

	take_off(EquipmentSlot::GIRDLE);
	EXPECT_EQ(player->get_strength(), BASE_STRENGTH);
}

// A buff that ends while gauntlets are worn does not leave its value behind in the score.
TEST_F(StatBoostEquipmentTest, ABuffIsNotAbsorbedByEquipment)
{
	ctx.buffSystem->add_buff(*player, BuffType::STRENGTH, 19, 10, true);
	put_on("gauntlets_of_swimming_and_climbing");
	ASSERT_EQ(player->get_strength(), 21) << "the buff's 19, plus 2";

	take_off(EquipmentSlot::GAUNTLETS);
	ctx.buffSystem->remove_buff(*player, BuffType::STRENGTH);

	EXPECT_EQ(player->get_strength(), BASE_STRENGTH);
}

// Cursed gauntlets of fumbling will not come off: using them again uses no turn, they stay
// on, and their -2 Dexterity stays with them.
TEST_F(StatBoostEquipmentTest, CursedGauntletsThatWillNotComeOffKeepTheirPenalty)
{
	put_on("gauntlets_of_fumbling");
	Item* gauntlets = player->get_equipped_item(EquipmentSlot::GAUNTLETS);
	gauntlets->enhancement.blessing = BlessingStatus::CURSED;

	EXPECT_FALSE(use_item(*gauntlets->behavior, *gauntlets, *player, ctx)) << "a failed removal used a turn";
	EXPECT_EQ(player->get_equipped_item(EquipmentSlot::GAUNTLETS), gauntlets);
	EXPECT_EQ(player->get_dexterity(), 8) << "the curse's -2 was lifted while the gauntlets stayed on";
}

// The rule is the creature's, not the player's: a monster wearing a girdle has its Strength.
TEST_F(StatBoostEquipmentTest, AMonsterWearingAGirdleHasItsStrength)
{
	Creature ogre{ Vector2D{ 1, 1 }, ActorData{ TileRef{}, "ogre", 0 } };
	ogre.set_strength(BASE_STRENGTH);
	ogre.set_body_plan({ EquipmentSlot::GIRDLE });

	ogre.wear(ItemCreator::create("girdle_of_hill_giant_strength", ogre.position, *ctx.contentRegistry), EquipmentSlot::GIRDLE);

	EXPECT_EQ(ogre.get_strength(), 19);
}
