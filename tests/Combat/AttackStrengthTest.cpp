// file: AttackStrengthTest.cpp
// Strength on an attack, by what makes it (Player's Handbook, PDF page 181): always on
// a swing; on a bow only its penalty, since no bow here is specially made; never on a
// crossbow, "a machine, not the player character"; never on a sling, which the rule
// does not name (the owner's reading, 2026-09-19).
//
// Expected values are Table 1's rows (PDF pages 30-31): Strength 3 is -3 to hit and -1
// damage; 18/00 is +3 to hit and +6 damage.
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

	// The row of Table 1 a score and percentile read.
	StrengthAttributes row(int strength, int exceptional) const
	{
		return mock.data_manager.strength_for(strength, exceptional);
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
	constexpr std::array<Case, 9> FROM_PAGE_181{ {
		{ "a swing, 18/00", 18, 100, AttackKind::MELEE, "", 3, 6 },
		{ "a swing, 3", 3, 0, AttackKind::MELEE, "", -3, -1 },
		{ "a thrown or natural missile, 18/00", 18, 100, AttackKind::RANGED, "", 3, 6 },
		{ "a bow, 18/00", 18, 100, AttackKind::RANGED, "long_bow", 0, 0 },
		{ "a bow, 3", 3, 0, AttackKind::RANGED, "long_bow", -3, -1 },
		{ "a crossbow, 18/00", 18, 100, AttackKind::RANGED, "light_crossbow", 0, 0 },
		{ "a crossbow, 3", 3, 0, AttackKind::RANGED, "light_crossbow", 0, 0 },
		{ "a sling, 18/00", 18, 100, AttackKind::RANGED, "sling", 0, 0 },
		{ "a sling, 3", 3, 0, AttackKind::RANGED, "sling", 0, 0 },
	} };

	for (const Case& expected : FROM_PAGE_181)
	{
		const std::unique_ptr<Item> fired = expected.weaponKey.empty() ? nullptr : weapon(expected.weaponKey);
		const AttackStrength::Adjustment adjustment = AttackStrength::adjustment(row(expected.strength, expected.exceptional), expected.kind, fired.get());

		EXPECT_EQ(adjustment.hit, expected.hit) << expected.source;
		EXPECT_EQ(adjustment.damage, expected.damage) << expected.source;
	}
}

// The sling is its own class now, not a bow that would take the bow's penalty.
TEST_F(AttackStrengthTest, TheSlingIsASlingAndARangedWeapon)
{
	const std::unique_ptr<Item> sling = weapon("sling");

	EXPECT_EQ(sling->itemClass, ItemClass::SLING);
	EXPECT_TRUE(sling->is_ranged_weapon());
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
