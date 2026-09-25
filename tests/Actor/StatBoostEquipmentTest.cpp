// Checks what wearing and removing ability-raising equipment does to an ability score.
//
// What it is for. Gauntlets, girdles and amulets that raise an ability either add to it -
// gauntlets of swimming and climbing, +2 Strength - or set it, as a girdle of giant
// strength does: "The Strength gained is not cumulative with normal or magical Strength
// bonuses" (Dungeon Master's Guide, PDF page 966 of the 2e archive). While worn the score
// is the higher of the creature's own and the highest set value, plus every addition, the
// rule buffs already follow - except that under a girdle of giant strength only the
// penalties count. Taking an item off must return the score to exactly what it was, in
// whatever order items and buffs come and go.
//
// Items are put on the way the inventory screen does - use_item on the item - and taken
// off the way the equipment screen does, unequip_item on the slot.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=StatBoostEquipmentTest.*

#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <string_view>

#include <nlohmann/json.hpp>

#include "src/ArmorClass.h"
#include "src/BuffSystem.h"
#include "src/BuffType.h"
#include "src/Creature.h"
#include "src/EquipmentSlot.h"
#include "src/ExperienceReward.h"
#include "src/Game.h"
#include "src/HealthPool.h"
#include "src/InventoryData.h"
#include "src/InventoryOperations.h"
#include "src/Item.h"
#include "src/ItemCreator.h"
#include "src/Paths.h"
#include "src/Pickable.h"
#include "src/Player.h"
#include "src/StrengthAttributes.h"

class StatBoostEquipmentTest : public ::testing::Test
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
		player->set_strength(BASE_STRENGTH);
		player->set_dexterity(10);
		player->set_constitution(10);

		ctx = game.context();
		ctx.playerOwner = &player;
	}

	// Puts an item from the data into the pack and uses it, as the inventory screen does.
	void put_on(std::string_view key)
	{
		auto item = ItemCreator::create(key, player->position, ctx);
		Item* carried = item.get();
		[[maybe_unused]] const auto added = InventoryOperations::add_item_to_inventory(player->inventoryData, std::move(item), *player, *ctx.dataManager);
		ASSERT_TRUE(added.has_value()) << key << " did not fit in the pack";
		use_item(*carried->behavior, *carried, *player, ctx);
		ASSERT_NE(player->get_equipped_item(slot_of(key)), nullptr) << key << " was not put on";
	}

	// An adding item, built here because the data holds none: every Strength item it
	// carries sets the score outright. What these cases are about is the rule that a
	// bonus adds, not any particular item, so the slot is filled from the data and the
	// behaviour replaced - the same way a girdle of dwarvenkind is made below.
	void put_on_adding_gauntlets(int strengthBonus)
	{
		put_on("gauntlets_of_swimming_and_climbing");
		player->get_equipped_item(EquipmentSlot::GAUNTLETS)->behavior = Gauntlets{ .strBonus = strengthBonus };
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

// An item that adds Strength adds it once, not once per query.
TEST_F(StatBoostEquipmentTest, AnAddingItemAddsOnce)
{
	put_on_adding_gauntlets(2);

	EXPECT_EQ(player->get_strength(), BASE_STRENGTH + 2);
	EXPECT_EQ(player->get_strength(), BASE_STRENGTH + 2) << "asking twice added twice";
}

// Taking it off returns exactly what it gave.
TEST_F(StatBoostEquipmentTest, RemovingAnAddingItemRestoresTheScore)
{
	put_on_adding_gauntlets(2);
	ASSERT_EQ(player->get_strength(), BASE_STRENGTH + 2);

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
	put_on_adding_gauntlets(2);
	ASSERT_EQ(player->get_strength(), 21) << "the buff's 19, plus 2";

	take_off(EquipmentSlot::GAUNTLETS);
	ctx.buffSystem->remove_buff(*player, BuffType::STRENGTH);

	EXPECT_EQ(player->get_strength(), BASE_STRENGTH);
}

// A Strength buff adds nothing to a girdle of giant strength's 19.
TEST_F(StatBoostEquipmentTest, AGirdleTakesNoBonusFromABuff)
{
	put_on("girdle_of_hill_giant_strength");

	ctx.buffSystem->add_buff(*player, BuffType::STRENGTH, 1, 10, false);

	EXPECT_EQ(player->get_strength(), 19) << "a buff's +1 was added to the girdle's 19";
}

// Nor does another worn item: an adding pair of gauntlets raises the wearer's own
// score and adds nothing to the girdle's.
TEST_F(StatBoostEquipmentTest, AGirdleTakesNoBonusFromAnotherWornItem)
{
	put_on("girdle_of_hill_giant_strength");
	put_on_adding_gauntlets(2);

	EXPECT_EQ(player->get_strength(), 19) << "the gauntlets' +2 was added to the girdle's 19";
}

// The book removes bonuses only, so a penalty still lowers the girdle's 19.
TEST_F(StatBoostEquipmentTest, APenaltyStillLowersAGirdlesStrength)
{
	put_on("girdle_of_hill_giant_strength");

	ctx.buffSystem->add_buff(*player, BuffType::STRENGTH, -1, 10, false);

	EXPECT_EQ(player->get_strength(), 18);
}

// The rule is about the girdle's Strength alone: a Dexterity bonus still adds while it is worn.
TEST_F(StatBoostEquipmentTest, AGirdleLeavesOtherAbilitiesTheirBonuses)
{
	put_on("girdle_of_hill_giant_strength");

	ctx.buffSystem->add_buff(*player, BuffType::DEXTERITY, 1, 10, false);

	EXPECT_EQ(player->get_dexterity(), 11);
}

// A girdle of dwarvenkind adds Constitution (DMG, PDF page 966) and sets no Strength, so
// Strength bonuses still count while it is worn.
TEST_F(StatBoostEquipmentTest, AGirdleThatSetsNoStrengthLeavesTheBonuses)
{
	put_on_adding_gauntlets(2);
	put_on("girdle_of_hill_giant_strength");

	// Made a girdle of dwarvenkind, which the data does not hold.
	player->get_equipped_item(EquipmentSlot::GIRDLE)->behavior = Girdle{ .conBonus = 1 };

	EXPECT_EQ(player->get_strength(), BASE_STRENGTH + 2);
}

// Gauntlets of ogre power give "18/00 Strength" (DMG, PDF page 964): +3 to hit, +6 damage.
TEST_F(StatBoostEquipmentTest, OgreGauntletsGiveEighteenHundred)
{
	put_on("gauntlets_of_ogre_power");

	EXPECT_EQ(player->get_strength(), 18);
	EXPECT_EQ(player->get_exceptional_strength(), 100);
	const StrengthAttributes row = game.dataManager.strength_for(player->get_strength(), player->get_exceptional_strength());
	EXPECT_EQ(row.hitProb, 3);
	EXPECT_EQ(row.dmgAdj, 6);
}

// A fighter's own 18/50 is back when the gauntlets come off.
TEST_F(StatBoostEquipmentTest, RemovingOgreGauntletsGivesBackTheWearersOwnPercentile)
{
	player->set_strength(18);
	player->set_exceptional_strength(50);
	put_on("gauntlets_of_ogre_power");
	ASSERT_EQ(player->get_exceptional_strength(), 100);

	take_off(EquipmentSlot::GAUNTLETS);

	EXPECT_EQ(player->get_exceptional_strength(), 50);
}

// An item that sets 18 with no percentile leaves the wearer's own 18/50 alone.
TEST_F(StatBoostEquipmentTest, AnEighteenWithoutAPercentileKeepsTheWearersOwn)
{
	player->set_strength(18);
	player->set_exceptional_strength(50);

	put_on("amulet_of_ogre_power");

	EXPECT_EQ(player->get_exceptional_strength(), 50);
}

// Worn ogre gauntlets are still 18/00 after the game is saved and loaded.
TEST_F(StatBoostEquipmentTest, WornOgreGauntletsSurviveASaveAndALoad)
{
	put_on("gauntlets_of_ogre_power");

	nlohmann::json saved;
	player->save(saved);
	auto loaded = std::make_unique<Player>(Vector2D{ 0, 0 });
	loaded->healthPool = std::make_unique<HealthPool>(0);
	loaded->load(saved);

	EXPECT_EQ(loaded->get_exceptional_strength(), 100);
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

namespace
{
// Adds an item of the given weight to the pack past its weight limit, as starting gear or a
// load is added, so the pack is as heavy as a test needs.
void load_pack(Player& player, int weight, GameContext& ctx)
{
	auto burden = ItemCreator::create("dagger", player.position, ctx);
	burden->enhancement.weight = weight;
	[[maybe_unused]] const auto added = InventoryOperations::add_item(player.inventoryData, std::move(burden));
	ASSERT_TRUE(added.has_value());
}

// Whether the floor holds this item at the given tile.
bool is_on_floor(const FloorInventory& floor, const Item* item, Vector2D tile)
{
	auto is_it = [item, tile](const std::unique_ptr<Item>& lying)
	{
		return lying.get() == item && lying->position == tile;
	};
	return std::ranges::any_of(floor.items, is_it);
}

} // namespace

// A girdle carries Strength 12's limit of 60 up to 19's 95. Taken off under a load of 80,
// the pack cannot take it back, so it is set down at the wearer's feet - never destroyed.
TEST_F(StatBoostEquipmentTest, AGirdleTakenOffUnderTooHeavyALoadIsSetDown)
{
	// Filled to exactly what this player could carry unaided, so the girdle's own pound
	// is one too many the moment its giant Strength stops counting. Taken before it goes
	// on, because wearing it is what raises the capacity.
	const int unaidedCapacity = InventoryOperations::get_max_weight(*player, *ctx.dataManager);
	put_on("girdle_of_hill_giant_strength");
	const Item* girdle = player->get_equipped_item(EquipmentSlot::GIRDLE);
	load_pack(*player, unaidedCapacity, ctx);
	// Walked elsewhere since the girdle was picked up, so its old position is not the answer.
	player->position = Vector2D{ 3, 4 };

	EXPECT_TRUE(player->unequip_item(EquipmentSlot::GIRDLE, ctx));

	EXPECT_EQ(player->get_equipped_item(EquipmentSlot::GIRDLE), nullptr);
	EXPECT_TRUE(is_on_floor(*ctx.floorInventory, girdle, player->position)) << "the girdle was destroyed";
}

// A full pack refuses whatever the weight: the item goes to the floor.
TEST_F(StatBoostEquipmentTest, AnItemTakenOffIntoAFullPackIsSetDown)
{
	put_on("gauntlets_of_ogre_power");
	const Item* gauntlets = player->get_equipped_item(EquipmentSlot::GAUNTLETS);
	while (!InventoryOperations::is_inventory_full(player->inventoryData))
	{
		load_pack(*player, 0, ctx);
	}

	EXPECT_TRUE(player->unequip_item(EquipmentSlot::GAUNTLETS, ctx));

	EXPECT_TRUE(is_on_floor(*ctx.floorInventory, gauntlets, player->position)) << "the gauntlets were destroyed";
}

// With nowhere to put it - a full pack and a full floor - the item stays on.
TEST_F(StatBoostEquipmentTest, AnItemWithNowhereToGoStaysOn)
{
	put_on("gauntlets_of_ogre_power");
	const Item* gauntlets = player->get_equipped_item(EquipmentSlot::GAUNTLETS);
	while (!InventoryOperations::is_inventory_full(player->inventoryData))
	{
		load_pack(*player, 0, ctx);
	}
	FloorInventory crowded{ 1 };
	[[maybe_unused]] const auto placed = InventoryOperations::add_item(crowded, ItemCreator::create("dagger", player->position, ctx));
	ctx.floorInventory = &crowded;

	EXPECT_FALSE(player->unequip_item(EquipmentSlot::GAUNTLETS, ctx));

	EXPECT_EQ(player->get_equipped_item(EquipmentSlot::GAUNTLETS), gauntlets) << "the gauntlets came off into nowhere";
	EXPECT_EQ(player->get_strength(), 18);
}

// The rule is the creature's, not the player's: a monster wearing a girdle has its Strength.
TEST_F(StatBoostEquipmentTest, AMonsterWearingAGirdleHasItsStrength)
{
	Creature ogre{ Vector2D{ 1, 1 }, ActorData{ TileRef{}, "ogre", 0 } };
	ogre.set_strength(BASE_STRENGTH);
	ogre.set_body_plan({ EquipmentSlot::GIRDLE });

	ogre.wear(ItemCreator::create("girdle_of_hill_giant_strength", ogre.position, ctx), EquipmentSlot::GIRDLE);

	EXPECT_EQ(ogre.get_strength(), 19);
}
