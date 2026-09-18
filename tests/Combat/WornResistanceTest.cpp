// file: WornResistanceTest.cpp
// Fire and cold resistance by the Dungeon Master's Guide, 2nd edition.
//
// Ring of Fire Resistance: magical fire is "saved against with a +4 bonus to the
// die roll, and all damage dice are calculated at -2 per die, but each die is
// never less than 1 in any event". Ring of Warmth: "a saving throw bonus of +2
// versus cold-based attacks, and reduces damage sustained by -1 per die". Helm
// of Brilliance: "protected as if a double-strength fire resistance ring were
// worn, but this protection cannot be augmented by further magical means".
//
// So resistance is a strength in rings - none, one, two - and it is the greatest
// of what is worn and what was drunk, never a sum. It is read from the slots
// when the hit lands; nothing is copied anywhere.

#include <gtest/gtest.h>

#include "src/Creature.h"
#include "src/EquipmentSlot.h"
#include "src/Item.h"
#include "src/Player.h"
#include "src/DamageInfo.h"
#include "src/DamageResolver.h"
#include "src/ExperienceReward.h"
#include "src/GameContext.h"
#include "src/Paths.h"
#include "src/ItemCreator.h"
#include "src/BuffSystem.h"
#include "src/BuffType.h"
#include "tests/mocks/MockGameContext.h"

class WornResistanceTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ItemCreator::load(Paths::ITEMS);
		ctx = mock.to_game_context();
		ctx.buffSystem = &buffs;

		player = std::make_unique<Player>(Vector2D{ 0, 0 });
		player->experienceReward = std::make_unique<ExperienceReward>(0);
		player->armorClass = std::make_unique<ArmorClass>(10);
		player->healthPool = std::make_unique<HealthPool>(100);
		// Strength sets what the pack can hold; without it a removed ring has
		// nowhere to go and unequipping asserts.
		player->set_strength(10);
		ctx.playerOwner = &player;
	}

	void wear(std::string_view key, EquipmentSlot slot)
	{
		player->equip_item(ItemCreator::create(key, Vector2D{ 0, 0 }, mock.content_registry), slot, ctx);
	}

	// A plain sword carrying only an enhancement's fire resistance.
	std::unique_ptr<Item> sword_of_fire_resistance()
	{
		auto item = std::make_unique<Item>(Vector2D{}, ActorData{});
		item->actorData.name = "sword of fire resistance";
		item->actorData.tile = TileRef{};
		item->itemClass = ItemClass::SWORD;
		item->behavior = Weapon{};
		item->enhancement.fireResistance = 50;
		return item;
	}

	int fire_strength() { return DamageResolver::resistance_strength(DamageType::FIRE, *player, ctx); }
	int cold_strength() { return DamageResolver::resistance_strength(DamageType::COLD, *player, ctx); }

	MockGameContext mock{};
	GameContext ctx{};
	BuffSystem buffs{};
	std::unique_ptr<Player> player{};
};

// A ring of fire resistance is one ring's worth against fire, and nothing against cold.
TEST_F(WornResistanceTest, ARingOfFireResistanceIsOneRing)
{
	wear("ring_of_fire_resistance", EquipmentSlot::RIGHT_RING);

	EXPECT_EQ(fire_strength(), 1);
	EXPECT_EQ(cold_strength(), 0);
}

// The book's cold ring is the ring of warmth: one ring against cold, in either hand.
TEST_F(WornResistanceTest, ARingOfWarmthIsOneRingAgainstCold)
{
	wear("ring_of_cold_resistance", EquipmentSlot::LEFT_RING);

	EXPECT_EQ(cold_strength(), 1);
	EXPECT_EQ(fire_strength(), 0);
}

// Taken off, it protects nothing: the answer is read from what is worn now.
TEST_F(WornResistanceTest, ARemovedRingProtectsNothing)
{
	wear("ring_of_fire_resistance", EquipmentSlot::RIGHT_RING);
	player->unequip_item(EquipmentSlot::RIGHT_RING, ctx);

	EXPECT_EQ(fire_strength(), 0);
}

// The helm of brilliance is a double-strength ring against fire.
TEST_F(WornResistanceTest, TheHelmOfBrillianceIsTwoRings)
{
	wear("helm_of_brilliance", EquipmentSlot::HEAD);

	EXPECT_EQ(fire_strength(), 2);
}

// A drunk potion is a ring's worth, and with a ring it is still one, never two.
TEST_F(WornResistanceTest, ARingAndAPotionAreOneRing)
{
	wear("ring_of_fire_resistance", EquipmentSlot::RIGHT_RING);
	buffs.add_buff(*player, BuffType::FIRE_RESISTANCE, 1, 10, false);

	EXPECT_EQ(fire_strength(), 1);
}

// The helm's protection "cannot be augmented": with a potion it is still two.
TEST_F(WornResistanceTest, TheHelmCannotBeAugmented)
{
	wear("helm_of_brilliance", EquipmentSlot::HEAD);
	buffs.add_buff(*player, BuffType::FIRE_RESISTANCE, 1, 10, false);

	EXPECT_EQ(fire_strength(), 2);
}

// The game's own enhancement of fire resistance is read as a ring's worth.
TEST_F(WornResistanceTest, AWornEnhancementIsOneRing)
{
	player->equip_item(sword_of_fire_resistance(), EquipmentSlot::RIGHT_HAND, ctx);

	EXPECT_EQ(fire_strength(), 1);
}

// The save bonuses: +4 per ring against fire, +2 per ring against cold.
TEST_F(WornResistanceTest, SaveBonusesFollowTheTwoEntries)
{
	wear("ring_of_fire_resistance", EquipmentSlot::RIGHT_RING);
	wear("ring_of_cold_resistance", EquipmentSlot::LEFT_RING);

	EXPECT_EQ(DamageResolver::save_bonus_against(DamageType::FIRE, *player, ctx), 4);
	EXPECT_EQ(DamageResolver::save_bonus_against(DamageType::COLD, *player, ctx), 2);
	EXPECT_EQ(DamageResolver::save_bonus_against(DamageType::LIGHTNING, *player, ctx), 0);
}

// The helm's save bonus is the double ring's: +8.
TEST_F(WornResistanceTest, TheHelmSavesAtPlusEight)
{
	wear("helm_of_brilliance", EquipmentSlot::HEAD);

	EXPECT_EQ(DamageResolver::save_bonus_against(DamageType::FIRE, *player, ctx), 8);
}

// The per-die arithmetic, on the resolver alone.
TEST(ReduceDiceTest, FireTakesTwoPerRingOffEveryDieAndNeverBelowOne)
{
	EXPECT_EQ(DamageResolver::reduce_dice({ 6, 6, 6 }, DamageType::FIRE, 0).hit_points(), 18);
	EXPECT_EQ(DamageResolver::reduce_dice({ 6, 6, 6 }, DamageType::FIRE, 1).hit_points(), 12);
	EXPECT_EQ(DamageResolver::reduce_dice({ 6, 6, 6 }, DamageType::FIRE, 2).hit_points(), 6);
	EXPECT_EQ(DamageResolver::reduce_dice({ 1, 2, 3 }, DamageType::FIRE, 1).hit_points(), 3) << "1, 1 and 1: a die never goes below one";
}

TEST(ReduceDiceTest, ColdTakesOnePerRingOffEveryDie)
{
	EXPECT_EQ(DamageResolver::reduce_dice({ 6, 6, 6 }, DamageType::COLD, 1).hit_points(), 15);
	EXPECT_EQ(DamageResolver::reduce_dice({ 1, 1, 1 }, DamageType::COLD, 1).hit_points(), 3);
}

// A type the book was not read for is summed as rolled, whatever the strength.
TEST(ReduceDiceTest, OtherTypesAreSummedAsRolled)
{
	EXPECT_EQ(DamageResolver::reduce_dice({ 6, 6, 6 }, DamageType::LIGHTNING, 2).hit_points(), 18);
}
