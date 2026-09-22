// file: AttackStrengthTest.cpp
// Strength on an attack, by what makes it (Player's Handbook, PDF page 181): always on
// a swing; on an ordinary bow only its penalty; never on a crossbow, "a machine, not the
// player character"; never on a sling, which the rule does not name (the owner's
// reading, 2026-09-19). A bow specially made for a Strength gives that Strength's row and
// needs it to be drawn: the composite bow, made for 18, is the first Baldur's Gate's -
// +1 to hit, +2 damage, "Requires: 18 Strength" - the owner's exception.
//
// Expected values are Table 1's rows (PDF pages 30-31): Strength 3 is -3 to hit and -1
// damage; 18 is +1 and +2; 18/00 is +3 and +6.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=AttackStrengthTest.*

#include <gtest/gtest.h>

#include <array>
#include <memory>
#include <string_view>

#include "src/AttackKind.h"
#include "src/AttackStrength.h"
#include "src/Creature.h"
#include "src/EquipmentSlot.h"
#include "src/ExperienceReward.h"
#include "src/Item.h"
#include "src/ItemClassification.h"
#include "src/ItemCreator.h"
#include "src/MonsterAttacker.h"
#include "src/Pickable.h"
#include "src/Player.h"
#include "src/PlayerAttacker.h"
#include "tests/mocks/MockGameContext.h"

namespace
{

constexpr int STARTING_HP = 100;
constexpr int DIE_ROLLED = 3;

} // namespace

class AttackStrengthTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();

		player = std::make_unique<Player>(Vector2D{ 0, 0 });
		player->experienceReward = std::make_unique<ExperienceReward>(0);
		player->armorClass = std::make_unique<ArmorClass>(10);
		player->healthPool = std::make_unique<HealthPool>(STARTING_HP);
		player->attacker = std::make_unique<PlayerAttacker>(*player);
		player->set_thaco(20);
		player->set_dr(0);
		player->set_strength(10);
		player->set_dexterity(10);
		ctx.playerOwner = &player;

		target = std::make_unique<Creature>(Vector2D{ 0, 1 }, ActorData{ TileRef{}, "goblin", 1 });
		target->experienceReward = std::make_unique<ExperienceReward>(0);
		target->armorClass = std::make_unique<ArmorClass>(10);
		target->healthPool = std::make_unique<HealthPool>(STARTING_HP);
		target->set_dr(0);
	}

	// A weapon of the class, loaded from the item data as the game makes it.
	std::unique_ptr<Item> weapon(std::string_view key)
	{
		return ItemCreator::create(key, Vector2D{ 0, 0 }, ctx);
	}

	// The player at this Strength, holding the weapon in the slot.
	void arm_player(int strength, int exceptional, std::string_view key, EquipmentSlot slot)
	{
		player->set_strength(strength);
		player->set_exceptional_strength(exceptional);
		ASSERT_TRUE(player->equip_item(weapon(key), slot, ctx));
	}

	// The player at Strength 12 in a girdle of hill giant strength and gauntlets of ogre power.
	void wear_girdle_and_gauntlets()
	{
		player->set_strength(12);
		ASSERT_TRUE(player->equip_item(weapon("girdle_of_hill_giant_strength"), EquipmentSlot::GIRDLE, ctx));
		ASSERT_TRUE(player->equip_item(weapon("gauntlets_of_ogre_power"), EquipmentSlot::GAUNTLETS, ctx));
		ASSERT_EQ(player->get_strength(), 19) << "the girdle's 19 did not decide the score";
	}

	// Hit points the target lost to one attack whose d20 and damage die are scripted.
	int damage_from(Creature& attacker, Creature& defender, AttackKind kind, int d20)
	{
		const int before = defender.get_hp();
		mock.dice.set_next_d20(d20);
		mock.dice.set_next_roll(DIE_ROLLED);
		attacker.attacker->attack(defender, kind, ctx);
		return before - defender.get_hp();
	}

	MockGameContext mock{};
	GameContext ctx{};
	std::unique_ptr<Player> player{};
	std::unique_ptr<Creature> target{};
};

// The rule itself, source by source.
TEST_F(AttackStrengthTest, EachSourceTakesItsPartOfTheRow)
{
	struct Case
	{
		std::string_view source{};
		int strength{ 0 };
		int exceptional{ 0 };
		AttackKind kind{ AttackKind::MELEE };
		std::string_view weaponKey{};
		int hit{ 0 };
		int damage{ 0 };
	};
	constexpr std::array<Case, 11> FROM_PAGE_181{ {
		{ "a swing, 18/00", 18, 100, AttackKind::MELEE, "", 3, 6 },
		{ "a swing, 3", 3, 0, AttackKind::MELEE, "", -3, -1 },
		{ "a thrown or natural missile, 18/00", 18, 100, AttackKind::RANGED, "", 3, 6 },
		{ "a bow, 18/00", 18, 100, AttackKind::RANGED, "long_bow", 0, 0 },
		{ "a bow, 3", 3, 0, AttackKind::RANGED, "long_bow", -3, -1 },
		{ "a crossbow, 18/00", 18, 100, AttackKind::RANGED, "light_crossbow", 0, 0 },
		{ "a crossbow, 3", 3, 0, AttackKind::RANGED, "light_crossbow", 0, 0 },
		{ "a sling, 18/00", 18, 100, AttackKind::RANGED, "sling", 0, 0 },
		{ "a sling, 3", 3, 0, AttackKind::RANGED, "sling", 0, 0 },
		{ "a composite bow, 18", 18, 0, AttackKind::RANGED, "composite_bow", 1, 2 },
		{ "a composite bow, 18/00", 18, 100, AttackKind::RANGED, "composite_bow", 1, 2 },
	} };

	for (const Case& expected : FROM_PAGE_181)
	{
		Creature attacker{ Vector2D{ 0, 0 }, ActorData{ TileRef{}, "attacker", 0 } };
		attacker.set_strength(expected.strength);
		attacker.set_exceptional_strength(expected.exceptional);
		const std::unique_ptr<Item> fired = expected.weaponKey.empty() ? nullptr : weapon(expected.weaponKey);
		const AttackStrength::Adjustment adjustment = AttackStrength::adjustment(
			mock.data_manager,
			attacker,
			expected.kind,
			fired.get());

		EXPECT_EQ(adjustment.hit, expected.hit) << expected.source;
		EXPECT_EQ(adjustment.damage, expected.damage) << expected.source;
	}
}

// Gauntlets of ogre power "add a +3 bonus to attack rolls and a +6 bonus to damage" to a
// blow struck or hurled (DMG, PDF page 964), and a girdle of giant strength keeps them
// beside its own row (page 966). A hill giant's 19 is +3 and +7, so the two are +6 and
// +13. A bow takes neither, as it takes no strong arm's bonus.
TEST_F(AttackStrengthTest, OgreGauntletsBesideAGirdleAddTheirOwnRow)
{
	wear_girdle_and_gauntlets();
	const std::unique_ptr<Item> longBow = weapon("long_bow");

	struct Case
	{
		std::string_view source{};
		AttackKind kind{ AttackKind::MELEE };
		const Item* fired{ nullptr };
		int hit{ 0 };
		int damage{ 0 };
	};
	const std::array<Case, 3> FROM_PAGES_964_AND_966{ {
		{ "a swing", AttackKind::MELEE, nullptr, 6, 13 },
		{ "a hurled or natural missile", AttackKind::RANGED, nullptr, 6, 13 },
		{ "a bow", AttackKind::RANGED, longBow.get(), 0, 0 },
	} };

	for (const Case& expected : FROM_PAGES_964_AND_966)
	{
		const AttackStrength::Adjustment adjustment = AttackStrength::adjustment(
			mock.data_manager,
			*player,
			expected.kind,
			expected.fired);

		EXPECT_EQ(adjustment.hit, expected.hit) << expected.source;
		EXPECT_EQ(adjustment.damage, expected.damage) << expected.source;
	}
}

// Each alone gives its own row: the girdle's 19 is +3 and +7, the gauntlets' 18/00 is +3
// and +6.
TEST_F(AttackStrengthTest, AGirdleOrOgreGauntletsAloneGiveOnlyTheirOwnRow)
{
	player->set_strength(12);
	ASSERT_TRUE(player->equip_item(weapon("girdle_of_hill_giant_strength"), EquipmentSlot::GIRDLE, ctx));
	const AttackStrength::Adjustment girdleAlone = AttackStrength::adjustment(
		mock.data_manager,
		*player,
		AttackKind::MELEE,
		nullptr);
	EXPECT_EQ(girdleAlone.hit, 3);
	EXPECT_EQ(girdleAlone.damage, 7);

	ASSERT_TRUE(player->unequip_item(EquipmentSlot::GIRDLE, ctx));
	ASSERT_TRUE(player->equip_item(weapon("gauntlets_of_ogre_power"), EquipmentSlot::GAUNTLETS, ctx));
	const AttackStrength::Adjustment gauntletsAlone = AttackStrength::adjustment(
		mock.data_manager,
		*player,
		AttackKind::MELEE,
		nullptr);
	EXPECT_EQ(gauntletsAlone.hit, 3);
	EXPECT_EQ(gauntletsAlone.damage, 6);
}

// The exception is for gauntlets of ogre power: gauntlets of swimming and climbing leave
// a girdle's blow at its own +3 and +7.
TEST_F(AttackStrengthTest, OtherGauntletsBesideAGirdleAddNothing)
{
	player->set_strength(12);
	ASSERT_TRUE(player->equip_item(weapon("girdle_of_hill_giant_strength"), EquipmentSlot::GIRDLE, ctx));
	ASSERT_TRUE(player->equip_item(weapon("gauntlets_of_swimming_and_climbing"), EquipmentSlot::GAUNTLETS, ctx));

	const AttackStrength::Adjustment swing = AttackStrength::adjustment(
		mock.data_manager,
		*player,
		AttackKind::MELEE,
		nullptr);

	EXPECT_EQ(swing.hit, 3);
	EXPECT_EQ(swing.damage, 7);
}

// Through an attack: THAC0 20 against armour class 10 needs a 10, so +6 hits on a 4 and
// not a 3, and the blow adds 13.
TEST_F(AttackStrengthTest, ASwingInOgreGauntletsAndAGirdleTakesBoth)
{
	wear_girdle_and_gauntlets();
	ASSERT_TRUE(player->equip_item(weapon("long_sword"), EquipmentSlot::RIGHT_HAND, ctx));

	EXPECT_EQ(damage_from(*player, *target, AttackKind::MELEE, 3), 0) << "a 3 hit";
	EXPECT_EQ(damage_from(*player, *target, AttackKind::MELEE, 4), DIE_ROLLED + 13);
}

// The sling is its own class now, not a bow that would take the bow's penalty.
TEST_F(AttackStrengthTest, TheSlingIsASlingAndARangedWeapon)
{
	const std::unique_ptr<Item> sling = weapon("sling");

	EXPECT_EQ(sling->itemClass, ItemClass::SLING);
	EXPECT_TRUE(sling->is_ranged_weapon());
}

// The composite bow is made for 18; an ordinary bow and a potion for nothing.
TEST_F(AttackStrengthTest, TheCompositeBowIsMadeForEighteen)
{
	EXPECT_EQ(strength_rating_of(*weapon("composite_bow")), 18);
	EXPECT_EQ(strength_rating_of(*weapon("long_bow")), 0);
	EXPECT_EQ(strength_rating_of(*weapon("health_potion")), 0);
}

// Through an attack: made for 18, it hits on 9 where 10 is needed and adds two, even
// in an 18/00 arm, which an ordinary bow would not reward at all.
TEST_F(AttackStrengthTest, ACompositeArrowTakesTheRowItIsMadeFor)
{
	arm_player(18, 100, "composite_bow", EquipmentSlot::MISSILE_WEAPON);

	EXPECT_EQ(damage_from(*player, *target, AttackKind::RANGED, 8), 0) << "an 8 hit";
	EXPECT_EQ(damage_from(*player, *target, AttackKind::RANGED, 9), DIE_ROLLED + 2);
}

// "Requires: 18 Strength": a 17 cannot draw it, and says so; it stays in the pack.
TEST_F(AttackStrengthTest, AnArmBelowTheRatingCannotDrawIt)
{
	player->set_strength(17);

	EXPECT_FALSE(player->equip_item(weapon("composite_bow"), EquipmentSlot::MISSILE_WEAPON, ctx));
	EXPECT_EQ(player->get_equipped_item(EquipmentSlot::MISSILE_WEAPON), nullptr);
	EXPECT_EQ(player->inventoryData.items.size(), 1u) << "the bow left the pack";

	bool refused = false;
	for (size_t index = 0; index < mock.messages.get_stored_message_count(); ++index)
	{
		for (const auto& part : mock.messages.get_attack_message_at(index))
		{
			refused = refused || part.logMessageText.find("not strong enough") != std::string::npos;
		}
	}
	EXPECT_TRUE(refused) << "the refusal said nothing";
}

// At the rating it draws.
TEST_F(AttackStrengthTest, AnArmAtTheRatingCanDrawIt)
{
	player->set_strength(18);

	EXPECT_TRUE(player->equip_item(weapon("composite_bow"), EquipmentSlot::MISSILE_WEAPON, ctx));
}

// "Strength 18 to use it" - the owner's ruling for the composite bow, and to use is not
// only to pick up. An arm that met the rating when it equipped the bow and has since lost
// the Strength - a girdle of giant strength taken off - draws nothing.
TEST_F(AttackStrengthTest, AnArmThatLosesItsStrengthCannotDrawTheBowItHolds)
{
	player->set_strength(12);
	ASSERT_TRUE(player->equip_item(weapon("girdle_of_hill_giant_strength"), EquipmentSlot::GIRDLE, ctx));
	ASSERT_EQ(player->get_strength(), 19) << "the girdle did not raise Strength";
	ASSERT_TRUE(player->equip_item(weapon("composite_bow"), EquipmentSlot::MISSILE_WEAPON, ctx));
	ASSERT_TRUE(player->unequip_item(EquipmentSlot::GIRDLE, ctx));
	ASSERT_EQ(player->get_strength(), 12) << "taking the girdle off did not lower Strength";

	EXPECT_EQ(damage_from(*player, *target, AttackKind::RANGED, 20), 0)
		<< "a Strength 12 arm drew a bow made for 18";
}

// A monster holds a rated bow the same way, whoever put it in its hands.
TEST_F(AttackStrengthTest, AMonsterTooWeakForItsBowDrawsNothing)
{
	Creature archer{ Vector2D{ 0, 1 }, ActorData{ TileRef{}, "archer", 1 } };
	archer.experienceReward = std::make_unique<ExperienceReward>(0);
	archer.armorClass = std::make_unique<ArmorClass>(10);
	archer.healthPool = std::make_unique<HealthPool>(STARTING_HP);
	archer.attacker = std::make_unique<MonsterAttacker>(archer, DamageInfo{ "1d6", DamageType::PHYSICAL });
	archer.set_thaco(20);
	archer.set_strength(12);
	archer.set_dexterity(10);
	archer.set_body_plan({ EquipmentSlot::MISSILE_WEAPON });
	archer.wear(weapon("composite_bow"), EquipmentSlot::MISSILE_WEAPON);

	EXPECT_EQ(damage_from(archer, *player, AttackKind::RANGED, 20), 0)
		<< "a Strength 12 monster drew a bow made for 18";
}

// A saved bow keeps what it was made for.
TEST_F(AttackStrengthTest, TheRatingSurvivesASave)
{
	json saved;
	weapon("composite_bow")->save(saved);

	Item loaded{ Vector2D{ 0, 0 }, ActorData{ TileRef{}, "", 0 } };
	loaded.load(saved);

	EXPECT_EQ(strength_rating_of(loaded), 18);
}

// Through an attack: an 18/00 arrow does its die alone, where the same arm swinging a
// sword adds six.
TEST_F(AttackStrengthTest, AStrongArchersArrowCarriesNoStrength)
{
	arm_player(18, 100, "long_bow", EquipmentSlot::MISSILE_WEAPON);
	ASSERT_TRUE(player->equip_item(weapon("long_sword"), EquipmentSlot::RIGHT_HAND, ctx));

	EXPECT_EQ(damage_from(*player, *target, AttackKind::RANGED, 20), DIE_ROLLED);
	EXPECT_EQ(damage_from(*player, *target, AttackKind::MELEE, 20), DIE_ROLLED + 6);
}

// A Strength 3 archer needs 13 where 10 would do, and takes a point off the arrow.
TEST_F(AttackStrengthTest, AWeakArchersPenaltyReachesTheRollAndTheDamage)
{
	arm_player(3, 0, "long_bow", EquipmentSlot::MISSILE_WEAPON);

	EXPECT_EQ(damage_from(*player, *target, AttackKind::RANGED, 12), 0) << "a 12 hit through the -3";
	EXPECT_EQ(damage_from(*player, *target, AttackKind::RANGED, 13), DIE_ROLLED - 1);
}

// A crossbow shoots the same for a weak arm as a strong one.
TEST_F(AttackStrengthTest, ACrossbowIgnoresAWeakArm)
{
	arm_player(3, 0, "light_crossbow", EquipmentSlot::MISSILE_WEAPON);

	EXPECT_EQ(damage_from(*player, *target, AttackKind::RANGED, 10), DIE_ROLLED);
}

// A monster's bow is read from its own missile slot, so the rule reaches it too.
TEST_F(AttackStrengthTest, AMonsterArcherFollowsTheSameRule)
{
	Creature archer{ Vector2D{ 0, 1 }, ActorData{ TileRef{}, "archer", 1 } };
	archer.experienceReward = std::make_unique<ExperienceReward>(0);
	archer.armorClass = std::make_unique<ArmorClass>(10);
	archer.healthPool = std::make_unique<HealthPool>(STARTING_HP);
	archer.attacker = std::make_unique<MonsterAttacker>(archer, DamageInfo{ "1d6", DamageType::PHYSICAL });
	archer.set_thaco(20);
	archer.set_strength(18);
	archer.set_exceptional_strength(100);
	archer.set_dexterity(10);
	archer.set_body_plan({ EquipmentSlot::MISSILE_WEAPON });
	archer.wear(weapon("long_bow"), EquipmentSlot::MISSILE_WEAPON);

	EXPECT_EQ(damage_from(archer, *player, AttackKind::RANGED, 20), DIE_ROLLED);
}
